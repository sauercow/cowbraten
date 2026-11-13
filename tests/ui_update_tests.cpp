#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/RenderInterface.h>

#include <iostream>

namespace
{
    int failures = 0;
    int updates = 0;

    void check(bool condition, const char *name)
    {
        std::cout << (condition ? "PASS  " : "FAIL  ") << name << '\n';
        if(!condition) ++failures;
    }

    class CountingElement : public Rml::Element
    {
    public:
        explicit CountingElement(const Rml::String &tag) : Rml::Element(tag) {}
        void OnUpdate() override { ++updates; }
    };

    class NullRenderer : public Rml::RenderInterface
    {
    public:
        Rml::CompiledGeometryHandle CompileGeometry(
            Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override { return 1; }
        void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override {}
        void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}
        Rml::TextureHandle LoadTexture(Rml::Vector2i &, const Rml::String &) override { return 0; }
        Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override { return 0; }
        void ReleaseTexture(Rml::TextureHandle) override {}
        void EnableScissorRegion(bool) override {}
        void SetScissorRegion(Rml::Rectanglei) override {}
    };
}

int main()
{
    NullRenderer renderer;
    Rml::ElementInstancerGeneric<CountingElement> counter_instancer;
    Rml::SetRenderInterface(&renderer);
    if(!Rml::Initialise()) return 1;
    Rml::Factory::RegisterElementInstancer("counter", &counter_instancer);
    auto *context = Rml::CreateContext("test", {800, 600});
    if(!context) return 1;

    Rml::String value = "old";
    auto constructor = context->CreateDataModel("test");
    constructor.Bind("value", &value);
    auto model = constructor.GetModelHandle();
    auto *menu = context->LoadDocumentFromMemory(
        "<rml><head><style>body { width: 100%; } div, counter { display: block; } counter { width: 10px; height: 10px; }</style></head>"
        "<body data-model='test'><div id='items'></div><div id='bound' data-attr-title='value'></div></body></rml>");
    auto *overlay = context->LoadDocumentFromMemory("<rml><body><counter /></body></rml>");
    if(!menu || !overlay) return 1;

    Rml::String items;
    for(int i = 0; i < 2000; ++i) items += "<counter />";
    menu->GetElementById("items")->SetInnerRML(items);
    menu->Show();
    overlay->Show();
    context->Update(false);
    menu->Hide();
    updates = 0;
    for(int frame = 0; frame < 20; ++frame) context->Update(false);
    check(updates == 20, "hidden large menu performs no element updates while overlay stays live");

    value = "new";
    model.DirtyVariable("value");
    context->SetDimensions({1000, 700});
    menu->GetElementById("items")->SetProperty("height", "37px");
    context->Update(false);
    check(menu->GetElementById("bound")->GetAttribute<Rml::String>("title", "") == "new",
        "data bindings synchronize while document is hidden");

    updates = 0;
    menu->Show();
    check(updates == 2000, "Show refreshes every cached element immediately");
    check(menu->GetBox().GetSize(Rml::BoxArea::Content).x == 1000,
        "reopened document uses the resized viewport");
    check(menu->GetElementById("items")->GetBox().GetSize(Rml::BoxArea::Content).y == 37,
        "reopened document applies deferred style and layout changes");
    updates = 0;
    context->Update(false);
    check(updates == 2001, "visible menu and overlay both update");

    menu->Hide();
    updates = 0;
    menu->UpdateDocument();
    check(updates == 2000, "explicit hidden document updates still work");
    updates = 0;
    context->Update();
    check(updates == 2001, "default RmlUi update behavior remains compatible");

    menu->Close();
    context->Update(false);
    check(context->GetNumDocuments() == 1, "closed hidden documents are released");
    Rml::Shutdown();
    return failures ? 1 : 0;
}
