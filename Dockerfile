FROM alpine:latest as builder

RUN apk add --upgrade \
    make \
    build-base \
    zlib-dev

# make sure to `make clean` in src/ and src/enet/ before
COPY ./src /src

RUN cd src && \
    make server && \
    strip cowbraten_server


FROM alpine:latest

RUN apk add --upgrade libstdc++

COPY --from=builder /src/cowbraten_server /server

# make sure you have a config!
COPY ./server-init.cfg /server-init.cfg

# Remove the next line when the server does not support p1xbraten-compatible slim maps or `/checkmaps` will fail.
RUN wget -O - https://static.p1x.pw/slim_ogzs.tar.gz | tar -xzof -

# remove -i part when not running on fly.io
CMD [ "/server", "-ifly-global-services" ]
