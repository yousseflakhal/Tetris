#ifndef SDLFORMUI_HPP
#define SDLFORMUI_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <optional>


enum class InputType {
    TEXT,
    NUMERIC,
    EMAIL,
    PASSWORD
};


struct UIConfig;

struct UITheme {
    SDL_Color backgroundColor     = { 100, 100, 100, 255 };
    SDL_Color hoverColor          = { 130, 130, 130, 255 };
    SDL_Color borderColor         = { 200, 200, 200, 255 };
    SDL_Color borderHoverColor    = { 255, 255, 255, 255 };
    SDL_Color textColor           = { 255, 255, 255, 255 };
    SDL_Color placeholderColor    = { 160, 160, 160, 255 };
    SDL_Color cursorColor         = { 255, 255, 255, 255 };
    SDL_Color sliderTrackColor    = { 80, 80, 80, 255 };
    SDL_Color sliderThumbColor    = { 180, 180, 255, 255 };
    SDL_Color checkboxTickColor   = { 255, 255, 255, 255 };
    TTF_Font* font;

    UITheme();
};

TTF_Font* getThemeFont(const UITheme& theme);


struct UITheme;
class UIConfig {
public:
    // NOTE: SDLFormUI does NOT take ownership. Caller must manage the lifetime (TTF_CloseFont).
    static void setDefaultFont(TTF_Font* font);
    static TTF_Font* getDefaultFont();

    static void setTheme(const UITheme& theme);
    static const UITheme& getTheme();
    static TTF_Font** getDefaultFontPtr();

private:
    static TTF_Font* defaultFont;
    static UITheme defaultTheme;
};


class UIElement {
public:
    SDL_Rect bounds;
    bool visible = true;
    bool enabled = true;

    virtual void handleEvent(const SDL_Event& e) = 0;
    virtual bool isHovered() const { return false; }
    virtual void update(float dt) = 0;
    virtual void render(SDL_Renderer* renderer) = 0;
    virtual void setPosition(int x, int y) { bounds.x = x; bounds.y = y; }
    virtual void setSize(int w, int h) { bounds.w = w; bounds.h = h; }
    virtual void setBounds(int x, int y, int w, int h) { bounds = {x, y, w, h}; }
    void setTheme(const UITheme& theme) { customTheme = theme; hasCustomTheme = true; }
    const UITheme& getTheme() const { return hasCustomTheme ? customTheme : UIConfig::getTheme(); }
    SDL_Point getPosition() const { return { bounds.x, bounds.y }; }
    SDL_Point getSize() const { return { bounds.w, bounds.h }; }
    virtual bool isInside(int x, int y) const {
    return x >= bounds.x && x <= bounds.x + bounds.w &&
           y >= bounds.y && y <= bounds.y + bounds.h;
    }
    void setEnabled(bool e) { enabled = e; }
    bool isEnabled() const { return enabled; }

    virtual ~UIElement() = default;

private:
    UITheme customTheme;
    bool hasCustomTheme = false;
};


class UIPopup : public UIElement {
public:
    UIPopup(int x, int y, int w, int h);
    void addChild(std::shared_ptr<UIElement> el);
    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(SDL_Renderer* renderer) override;

    std::vector<std::shared_ptr<UIElement>> children;

};


class UIButton;

class UIDialog : public UIPopup {
public:
    UIDialog(const std::string& title,
             const std::string& message,
             std::function<void()> onOk = nullptr,
             std::function<void()> onCancel = nullptr);

    void render(SDL_Renderer* renderer) override;
    void handleEvent(const SDL_Event& e) override;

    void close();

private:
    std::string title;
    std::string message;
    std::function<void()> onOk;
    std::function<void()> onCancel;

    std::shared_ptr<UIButton> okButton;
    std::shared_ptr<UIButton> cancelButton;
    bool ignoreNextClick = true;
};


namespace UIHelpers {
    void DrawFilledCircle(SDL_Renderer* renderer, int cx, int cy, int radius, SDL_Color color);
    void DrawCircleRing(SDL_Renderer* renderer, int cx, int cy, int radius, int thickness, SDL_Color color);
    void FillRoundedRect(SDL_Renderer* renderer,
                                  int x, int y, int w, int h,
                                  int radius, SDL_Color color);
    inline SDL_Color AdjustBrightness(SDL_Color c, int delta) {
        auto clamp = [](int v){ return std::max(0, std::min(255, v)); };
        return SDL_Color{ Uint8(clamp(c.r + delta)), Uint8(clamp(c.g + delta)),
                          Uint8(clamp(c.b + delta)), c.a };
    }
    inline SDL_Color WithAlpha(SDL_Color c, Uint8 a) { c.a = a; return c; }
    void DrawShadowRoundedRect(SDL_Renderer* r, const SDL_Rect& rect, int radius, int offset, Uint8 alpha);
    inline float RelativeLuma(SDL_Color c) {
        auto lin = [](float u){ u/=255.0f; return (u<=0.04045f)? u/12.92f : powf((u+0.055f)/1.055f, 2.4f); };
        float R = lin(c.r), G = lin(c.g), B = lin(c.b);
        return 0.2126f*R + 0.7152f*G + 0.0722f*B;
    }
    inline SDL_Color Lighten(SDL_Color c, int delta) { return AdjustBrightness(c, +std::abs(delta)); }
    inline SDL_Color Darken (SDL_Color c, int delta) { return AdjustBrightness(c, -std::abs(delta)); }

    inline SDL_Color PickHoverColor(SDL_Color bg) {
        float L = RelativeLuma(bg);
        if (L < 0.10f) {
            return Lighten(bg, 12);
        } else {
            return Darken(bg, 12);
        }
    }

    inline SDL_Color PickFocusRing(SDL_Color bg) {
        float L = RelativeLuma(bg);
        return (L > 0.85f) ? Darken(bg, 40) : Lighten(bg, 40);
    }

    void StrokeRoundedRectOutside(SDL_Renderer* r,
                                  const SDL_Rect& innerRect,
                                  int radius,
                                  int thickness,
                                  SDL_Color ringColor,
                                  SDL_Color innerBg);
    void DrawRoundStrokeLine(SDL_Renderer* r, float x1, float y1, float x2, float y2, float thickness, SDL_Color color);
    void DrawCheckmark(SDL_Renderer* r, const SDL_Rect& box, float thickness, SDL_Color color, float pad);
    inline SDL_Color RGBA(int r, int g, int b, int a = 255) {
        auto clip = [](int v){ return static_cast<Uint8>(std::clamp(v, 0, 255)); };
        return SDL_Color{ clip(r), clip(g), clip(b), clip(a) };
    }
}


class UIRadioGroup;

class UIRadioButton : public UIElement {
public:
    UIRadioButton(const std::string& label, int x, int y, int w, int h, UIRadioGroup* group, int id, TTF_Font* font = nullptr);

    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(SDL_Renderer* renderer) override;
    bool isHovered() const override;
    void setFont(TTF_Font* font);
    int getID() const;

    UIRadioButton* setFocusable(bool f) { focusable = f; return this; }
    bool isFocused() const { return focused; }

private:
    std::string label;
    int id;
    UIRadioGroup* group = nullptr;
    TTF_Font* font = nullptr;
    bool hovered = false;
    bool pressed = false;
    bool focused = false;
    bool focusable = true;
};


class UIRadioButton;

class UIRadioGroup {
    public:
        void addButton(std::shared_ptr<UIRadioButton> btn);
        void select(int id);
        int getSelectedID() const;

    private:
        std::vector<std::shared_ptr<UIRadioButton>> buttons;
        int selectedID = -1;
    };


class UIButton : public UIElement {
public:
    UIButton(const std::string& text, int x, int y, int w, int h, TTF_Font* f = nullptr);
    void setOnClick(std::function<void()> callback);
    void setText(const std::string& newText);
    const std::string& getText() const;
    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(SDL_Renderer* renderer) override;
    void setFont(TTF_Font* f);
    bool isHovered() const;
    UIButton* setTextColor(SDL_Color c);
    UIButton* setBackgroundColor(SDL_Color c);
    UIButton* setBorderColor(SDL_Color c);
    UIButton* setCornerRadius(int r) { cornerRadius = (r < 0 ? 0 : r); return this; }
    UIButton* setBorderThickness(int px) { borderPx = (px < 0 ? 0 : px); return this; }
    UIButton* setFocusable(bool f) { focusable = f; return this; }
    bool isFocused() const { return focused; }

private:
    std::string label;
    std::function<void()> onClick;
    bool hovered = false;
    bool pressed = false;
    TTF_Font* font = nullptr;
    std::optional<SDL_Color> customTextColor;
    std::optional<SDL_Color> customBgColor;
    std::optional<SDL_Color> customBorderColor;
    int cornerRadius = 4;
    int borderPx     = 1;
    bool focused = false;
    bool focusable = true;
    int pressOffset = 1;
};


class UILabel : public UIElement {
public:
    UILabel(const std::string& text, int x, int y, int w, int h, TTF_Font* font = nullptr);
    void render(SDL_Renderer* renderer) override;
    void update(float dt) override { (void)dt; }
    void handleEvent(const SDL_Event& e) override { (void)e; }

    UILabel* setColor(SDL_Color newColor);
    SDL_Color getColor() const;

private:
    std::string text;
    TTF_Font* font = nullptr;
    SDL_Color color = {255, 255, 255, 255};
};


class UICheckbox : public UIElement {
public:
    UICheckbox(const std::string& label, int x, int y, int w, int h, bool& bind, TTF_Font* f);

    void setFont(TTF_Font* f);
     UICheckbox* setTextColor(SDL_Color c)    { customTextColor   = c; hasCustomTextColor   = true; return this; }
    UICheckbox* setCheckedColor(SDL_Color c) { customCheckedColor= c; hasCustomCheckedColor= true; return this; }
    UICheckbox* setBoxBackground(SDL_Color c){ customBoxBgColor   = c; hasCustomBoxBgColor   = true; return this; }
    UICheckbox* setBorderColor(SDL_Color c)   { customBorderColor = c; hasCustomBorderColor = true; return this; }
    UICheckbox* setBorderThickness(int px) { borderPx = std::max(0, px); return this; }



    void handleEvent(const SDL_Event& e) override;
    bool isHovered() const override;
    void update(float dt) override;
    void render(SDL_Renderer* renderer) override;

private:
    std::string label;
    bool hovered = false;
    bool focused = false;
    bool focusable = true;
    std::reference_wrapper<bool> linkedValue;
    TTF_Font* font = nullptr;
    SDL_Color customTextColor{};    bool hasCustomTextColor    = false;
    SDL_Color customCheckedColor{}; bool hasCustomCheckedColor = false;
    SDL_Color customBoxBgColor{};   bool hasCustomBoxBgColor   = false;
    SDL_Color customBorderColor{};  bool hasCustomBorderColor  = false;
    int borderPx = 1;
};


class UITextField : public UIElement {
public:
    UITextField(const std::string& label, int x, int y, int w, int h, std::string& bind, int maxLen = 32);
    UITextField* setPlaceholder(const std::string& text);
    UITextField* setFont(TTF_Font* f);
    UITextField* setInputType(InputType type);

    bool isHovered() const override;
    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(SDL_Renderer* renderer) override;
    std::function<void(const std::string&)> onSubmit;
    UITextField* setOnSubmit(std::function<void(const std::string&)> cb) {
        onSubmit = std::move(cb);
        return this;
    }



private:
    std::string label;
    std::reference_wrapper<std::string> linkedText;
    int maxLength = 32;
    bool hovered = false;
    bool focused = false;
    Uint32 lastBlinkTime = 0;
    bool cursorVisible = true;
    std::string placeholder;
    SDL_Color placeholderColor = {160, 160, 160, 255};
    TTF_Font* font = nullptr;
    InputType inputType = InputType::TEXT;
    int cornerRadius = 10;
    int borderPx     = 1;
    Uint32 lastInputTicks = 0;
    Uint32 lastBlinkTicks = 0;
};



class UIComboBox : public UIElement {
public:
    UIComboBox(int x, int y, int w, int h, const std::vector<std::string>& options, int& selectedIndex);

    void setFont(TTF_Font* f);
    void setOnSelect(std::function<void(int)> callback);
    bool isHovered() const override;
    bool isExpanded() const;
    int  getItemCount() const;
    int  getItemHeight() const;
    const SDL_Rect& getBounds() const;

    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(SDL_Renderer* renderer) override;
    UIComboBox* setTextColor(SDL_Color c);

    UIComboBox* setFocusable(bool f) { focusable = f; return this; }
    bool isFocused() const { return focused; }

private:
    std::vector<std::string> options;
    std::reference_wrapper<int> selectedIndex;
    std::function<void(int)> onSelect;
    TTF_Font* font = nullptr;
    std::optional<SDL_Color> customTextColor;

    bool expanded = false;
    int  hoveredIndex = -1;
    bool hovered = false;

    bool focused = false;
    bool focusable = true;
    int  cornerRadius = 8;
};


class UISlider : public UIElement {
public:
    UISlider(const std::string& label, int x, int y, int w, int h, float& bind, float min, float max);

    void handleEvent(const SDL_Event& e) override;
    bool isHovered() const override;
    void update(float dt) override;
    void render(SDL_Renderer* renderer) override;

private:
    std::string label;
    std::reference_wrapper<float> linkedValue;
    float minVal = 0.0f, maxVal = 100.0f;

    bool hovered = false;
    bool focused = false;
    bool focusable = true;
    bool dragging = false;

    int thumbRadius = 9;
};


class UISpinner : public UIElement {
    public:
        UISpinner(int x, int y, int w, int h, int& bind, int min = 0, int max = 100, int step = 1);

        void setFont(TTF_Font* f);
        void setOnChange(std::function<void(int)> callback);
        bool isHovered() const;

        void handleEvent(const SDL_Event& e) override;
        void update(float dt) override;
        void render(SDL_Renderer* renderer) override;

    private:
        std::reference_wrapper<int> value;
        int minValue;
        int maxValue;
        int step;
        bool hoveredMinus = false;
        bool hoveredPlus = false;
        TTF_Font* font = nullptr;
        std::function<void(int)> onChange;
        enum class HeldButton { NONE, INCREMENT, DECREMENT };
        HeldButton heldButton = HeldButton::NONE;
        Uint32 pressStartTime = 0;
        Uint32 lastStepTime = 0;
};



class UITextArea : public UIElement {
public:
    UITextArea(const std::string& label, int x, int y, int w, int h, std::string& bind, int maxLen = 512);

    void setFont(TTF_Font* f);
    void setPlaceholder(const std::string& text);
    void updateCursorPosition();
    SDL_Rect getScrollbarRect() const;
    void renderScrollbar(SDL_Renderer* renderer);
    bool isScrollbarHovered() const;
    bool isScrollbarDragging() const;

    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(SDL_Renderer* renderer) override;
    bool isHovered() const override;
    int getWordCount() const;

private:
    std::vector<std::string> wrapTextToLines(const std::string& text, TTF_Font* font, int maxWidth);
    std::string label;
    std::reference_wrapper<std::string> linkedText;
    std::string placeholder;
    int maxLength;
    bool hovered = false;
    bool focused = false;
    TTF_Font* font = nullptr;
    Uint32 lastBlinkTime = 0;
    bool cursorVisible = true;
    size_t cursorPos = 0;
    bool scrollbarHovered = false;
    bool scrollbarDragging = false;
    int scrollbarDragStartY = 0;
    int scrollbarThumbStartOffset = 0;
    InputType inputType = InputType::TEXT;
    float scrollOffsetY = 0.0f;
    float contentHeight = 0.0f;
    int cursorX = 0, cursorY = 0;
};


class UIGroupBox : public UIElement {
public:
    UIGroupBox(const std::string& title, int x, int y, int w, int h);

    void addChild(std::shared_ptr<UIElement> child);
    const std::vector<std::shared_ptr<UIElement>>& getChildren() const;

    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(SDL_Renderer* renderer) override;

private:
    std::string title;
    std::vector<std::shared_ptr<UIElement>> children;
    TTF_Font* font = nullptr;
};



namespace FormUI {

class Layout {
public:
    Layout(int x, int y, int spacing = 10)
        : currentX(x), currentY(y), spacing(spacing) {}

    std::shared_ptr<UILabel> addLabel(const std::string& text, int width = 300, int height = 30);
    std::shared_ptr<UICheckbox> addCheckbox(const std::string& label, bool& value, int width = 300, int height = 30);
    std::shared_ptr<UISlider> addSlider(const std::string& label, float& value, float min, float max, int width = 300, int height = 40);
    std::shared_ptr<UITextField> addTextField(const std::string& label, std::string& bind, int maxLen = 32, int width = 300, int height = 40);
    std::shared_ptr<UIButton> addButton(const std::string& label, std::function<void()> onClick, int width, int height, TTF_Font* font);
    std::pair<std::shared_ptr<UILabel>, std::shared_ptr<UIButton>> addLabelButtonRow(
        const std::string& labelText,
        const std::string& buttonText,
        std::function<void()> onClick,
        int labelWidth = 200,
        int buttonWidth = 100,
        int height = 30,
        TTF_Font* labelFont = nullptr,
        TTF_Font* buttonFont = nullptr
    );
    std::shared_ptr<UIRadioGroup> addRadioGroup(
        const std::vector<std::pair<std::string, int>>& options,
        int& selectedID,
        int width = 300,
        int height = 30,
        int groupSpacing = 10
    );
    std::shared_ptr<UIComboBox> addComboBox(const std::vector<std::string>& options, int& selectedIndex, int width = 300, int height = 30);
    std::shared_ptr<UISpinner> addSpinner(int& bind, int min = 0, int max = 100, int step = 1, int width = 100, int height = 30);
    std::shared_ptr<UITextArea> addTextArea(const std::string& label, std::string& bind, int maxLen = 512, int width = 300, int height = 100);
    void setDefaultFont(TTF_Font* font) { defaultFont = font; }
    TTF_Font* getDefaultFont() const { return defaultFont; }


private:
    int currentX, currentY;
    int spacing;
    TTF_Font* defaultFont = nullptr;
};

}

class UIManager {
    public:
        void initCursors();
        void cleanupCursors();
        void addElement(std::shared_ptr<UIElement> el);
        void showPopup(std::shared_ptr<UIPopup> popup);
        std::shared_ptr<UIPopup> GetActivePopup();
        void closePopup();
        void checkCursorForElement(const std::shared_ptr<UIElement>& el, SDL_Cursor*& cursorToUse);
        void handleEvent(const SDL_Event& e);
        void update(float dt);
        void render(SDL_Renderer* renderer);


    private:
        std::vector<std::shared_ptr<UIElement>> elements;
        SDL_Cursor* arrowCursor = nullptr;
        SDL_Cursor* handCursor = nullptr;
        SDL_Cursor* ibeamCursor = nullptr;
        bool handCursorActive = false;
        std::shared_ptr<UIPopup> activePopup;
    };



namespace FormUI {
    // NOTE: SDLFormUI does not take ownership of the font.
    void Init(TTF_Font* defaultFont = nullptr);
    void Shutdown();

    std::shared_ptr<UIButton> Button( const std::string& label, int x, int y, int w, int h, std::function<void()> onClick = nullptr, TTF_Font* font = nullptr);
    std::shared_ptr<UICheckbox> Checkbox(const std::string& label, int x, int y, int w, int h, bool& bind, TTF_Font* font);
    std::shared_ptr<UILabel> Label(const std::string& text, int x, int y, int w, int h, TTF_Font* font = nullptr);
    std::shared_ptr<UISlider> Slider(const std::string& label, int x, int y, int w, int h, float& bind, float min, float max);
    std::shared_ptr<UITextField> TextField(const std::string& label, int x, int y, int w, int h, std::string& bind, int maxLen = 32);
    std::shared_ptr<UIComboBox> ComboBox(const std::vector<std::string>& options, int x, int y, int w, int h, int& selectedIndex, TTF_Font* font = nullptr);
    std::shared_ptr<UISpinner> Spinner(int x, int y, int w, int h, int& bind, int min = 0, int max = 100, int step = 1, TTF_Font* font = nullptr);
    std::shared_ptr<UITextArea> TextArea(const std::string& label, int x, int y, int w, int h, std::string& bind, int maxLen = 512);
    void AddElement(std::shared_ptr<UIElement> element);
    void ShowPopup(std::shared_ptr<UIPopup> popup);
    void ClosePopup();



    void HandleEvent(const SDL_Event& e);
    void Update();
    void Render(SDL_Renderer* renderer);
}

#ifdef SDLFORMUI_IMPLEMENTATION


UITheme::UITheme() : font(nullptr) {}

TTF_Font* getThemeFont(const UITheme& theme) {
    return theme.font ? theme.font : UIConfig::getDefaultFont();
}


TTF_Font* UIConfig::defaultFont = nullptr;
UITheme UIConfig::defaultTheme;

void UIConfig::setDefaultFont(TTF_Font* font) {
    defaultFont = font;
}

TTF_Font* UIConfig::getDefaultFont() {
    return defaultFont;
}

void UIConfig::setTheme(const UITheme& theme) {
    defaultTheme = theme;
}

const UITheme& UIConfig::getTheme() {
    return defaultTheme;
}

TTF_Font** UIConfig::getDefaultFontPtr() {
    return &defaultFont;
}


UIPopup::UIPopup(int x, int y, int w, int h) {
    bounds = { x, y, w, h };
}

void UIPopup::addChild(std::shared_ptr<UIElement> el) {
    children.push_back(el);
}

void UIPopup::handleEvent(const SDL_Event& e) {
    for (auto& child : children) {
        if (child)  child->handleEvent(e);
    }
}

void UIPopup::update(float dt) {
    for (auto& child : children) {
        child->update(dt);
    }
}

void UIPopup::render(SDL_Renderer* renderer) {
    for (auto& child : children) {
        child->render(renderer);
    }
}


UIDialog::UIDialog(const std::string& title,
                   const std::string& message,
                   std::function<void()> onOk,
                   std::function<void()> onCancel)
    : UIPopup((800 - 400) / 2, (600 - 200) / 2, 400, 200),
      title(title), message(message), onOk(onOk), onCancel(onCancel)
{
    const int dialogWidth = 400;
    const int dialogHeight = 200;
    bounds = {
        (800 - dialogWidth) / 2,
        (600 - dialogHeight) / 2,
        dialogWidth,
        dialogHeight
    };

    TTF_Font* font = UIConfig::getDefaultFont();
    int btnW = 100, btnH = 40, spacing = 20;
    int btnY = bounds.y + bounds.h - btnH - spacing;
    int okX = bounds.x + bounds.w / 2 - btnW - spacing / 2;
    int cancelX = bounds.x + bounds.w / 2 + spacing / 2;

    okButton = std::make_shared<UIButton>("OK", okX, btnY, btnW, btnH, font);
    cancelButton = std::make_shared<UIButton>("Cancel", cancelX, btnY, btnW, btnH, font);

    okButton->setOnClick([this, onOk]() {
        if (onOk) onOk();
        close();
    });

    cancelButton->setOnClick([this, onCancel]() {
        if (onCancel) onCancel();
        close();
    });

    addChild(okButton);
    addChild(cancelButton);
}

void UIDialog::render(SDL_Renderer* renderer) {
    const UITheme& theme = getTheme();
    TTF_Font* font = UIConfig::getDefaultFont();
    if (!font) return;

    SDL_SetRenderDrawColor(renderer, theme.backgroundColor.r, theme.backgroundColor.g, theme.backgroundColor.b, theme.backgroundColor.a);
    SDL_RenderFillRect(renderer, &bounds);

    SDL_SetRenderDrawColor(renderer, theme.borderColor.r, theme.borderColor.g, theme.borderColor.b, theme.borderColor.a);
    SDL_RenderDrawRect(renderer, &bounds);

    SDL_Surface* titleSurf = TTF_RenderText_Blended(font, title.c_str(), theme.textColor);
    SDL_Surface* msgSurf = TTF_RenderText_Blended(font, message.c_str(), theme.textColor);

    if (titleSurf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, titleSurf);
        SDL_Rect titleRect = {
            bounds.x + 20,
            bounds.y + 20,
            titleSurf->w,
            titleSurf->h
        };
        SDL_RenderCopy(renderer, tex, nullptr, &titleRect);
        SDL_FreeSurface(titleSurf);
        SDL_DestroyTexture(tex);
    }

    if (msgSurf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, msgSurf);
        SDL_Rect msgRect = {
            bounds.x + 20,
            bounds.y + 70,
            msgSurf->w,
            msgSurf->h
        };
        SDL_RenderCopy(renderer, tex, nullptr, &msgRect);
        SDL_FreeSurface(msgSurf);
        SDL_DestroyTexture(tex);
    }

    UIPopup::render(renderer);
}

void UIDialog::handleEvent(const SDL_Event& e) {
    if (ignoreNextClick) {
        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) return;
        ignoreNextClick = false;
    }
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        close();
        return;
    }
    UIPopup::handleEvent(e);
}

void UIDialog::close() {
    visible = false;
}


namespace UIHelpers {

void DrawFilledCircle(SDL_Renderer* renderer, int cx, int cy, int radius, SDL_Color color) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    const float threshold = 0.5f;
    const float maxDist = radius + threshold;

    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            float distance = std::hypotf(x, y);

            if (distance <= radius - threshold) {
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                SDL_RenderDrawPoint(renderer, cx + x, cy + y);
            } else if (distance <= maxDist) {
                float alpha = color.a * (1.0f - (distance - (radius - threshold)));
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, static_cast<Uint8>(alpha));
                SDL_RenderDrawPoint(renderer, cx + x, cy + y);
            }
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void DrawCircleRing(SDL_Renderer* renderer, int cx, int cy, int radius, int thickness, SDL_Color color) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    const int innerRadius = radius - thickness;
    const float feather = 0.5f;

    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            float distance = std::hypotf(x, y);

            if (distance >= innerRadius - feather && distance <= radius + feather) {
                float alpha = color.a;
                if (distance > radius - feather) {
                    alpha *= (radius + feather - distance) / (2 * feather);
                }
                if (distance < innerRadius + feather) {
                    alpha *= (distance - (innerRadius - feather)) / (2 * feather);
                }

                alpha = std::clamp(alpha, 0.0f, 255.0f);
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, static_cast<Uint8>(alpha));
                SDL_RenderDrawPoint(renderer, cx + x, cy + y);
            }
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

}

void UIHelpers::FillRoundedRect(SDL_Renderer* renderer,
                                         int x, int y, int w, int h,
                                         int radius, SDL_Color color) {
    SDL_BlendMode original_mode;
    SDL_GetRenderDrawBlendMode(renderer, &original_mode);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    SDL_Rect center = { x + radius, y, w - 2*radius, h };
    SDL_RenderFillRect(renderer, &center);
    SDL_Rect sides  = { x, y + radius, w, h - 2*radius };
    SDL_RenderFillRect(renderer, &sides);

    const int centers[4][2] = {
        { x + radius,     y + radius     },
        { x + w - radius, y + radius     },
        { x + radius,     y + h - radius },
        { x + w - radius, y + h - radius }
    };

    for (int corner = 0; corner < 4; ++corner) {
        int cx = centers[corner][0];
        int cy = centers[corner][1];

        int start_x = (corner % 2 == 0) ? x : x + w - radius;
        int end_x   = (corner % 2 == 0) ? x + radius : x + w;
        int start_y = (corner < 2) ? y : y + h - radius;
        int end_y   = (corner < 2) ? y + radius : y + h;

        for (int py = start_y; py < end_y; ++py) {
            for (int px = start_x; px < end_x; ++px) {
                float dx = px - cx + 0.5f;
                float dy = py - cy + 0.5f;
                float distance = sqrtf(dx*dx + dy*dy);

                if (distance <= radius - 0.5f) {
                    SDL_RenderDrawPoint(renderer, px, py);
                } else if (distance < radius + 0.5f) {
                    Uint8 alpha = (Uint8)(color.a * (1.0f - (distance - (radius - 0.5f))));
                    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
                    SDL_RenderDrawPoint(renderer, px, py);
                    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                }
            }
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, original_mode);
}

void UIHelpers::DrawShadowRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, int offset, Uint8 alpha) {
    SDL_Rect shadow = { rect.x + offset, rect.y + offset, rect.w, rect.h };
    SDL_Color sc = { 0, 0, 0, alpha };
    FillRoundedRect(renderer, shadow.x, shadow.y, shadow.w, shadow.h, radius, sc);
}

void UIHelpers::StrokeRoundedRectOutside(SDL_Renderer* renderer,
                                         const SDL_Rect& innerRect,
                                         int radius,
                                         int thickness,
                                         SDL_Color ringColor,
                                         SDL_Color innerBg) {
    SDL_Rect outer = {
        innerRect.x - thickness,
        innerRect.y - thickness,
        innerRect.w + 2*thickness,
        innerRect.h + 2*thickness
    };
    FillRoundedRect(renderer, outer.x, outer.y, outer.w, outer.h, radius + thickness, ringColor);

    FillRoundedRect(renderer, innerRect.x, innerRect.y, innerRect.w, innerRect.h, radius, innerBg);
}

void UIHelpers::DrawRoundStrokeLine(SDL_Renderer* r, float x1, float y1, float x2, float y2, float thickness, SDL_Color color) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = std::sqrt(dx*dx + dy*dy);
    if (len <= 0.0001f) {
        UIHelpers::DrawFilledCircle(r, (int)std::round(x1), (int)std::round(y1), (int)std::round(thickness * 0.45f), color);
        return;
    }
    float ux = dx / len, uy = dy / len;
    const float radius = thickness * 0.5f;
    const float step   = std::max(0.25f, std::min(0.35f, radius * 0.35f));
    const int   n      = (int)std::ceil(len / step);

    for (int i = 0; i <= n; ++i) {
        float t = (i * step);
        float px = x1 + ux * t;
        float py = y1 + uy * t;
        UIHelpers::DrawFilledCircle(r, (int)std::round(px), (int)std::round(py), (int)std::round(radius), color);
    }
}

void UIHelpers::DrawCheckmark(SDL_Renderer* r, const SDL_Rect& box, float thickness, SDL_Color color, float pad) {
    const float scaleX = 0.84f;
    const float scaleY = 0.84f;

    float x1 = box.x + pad;                float y1 = box.y + box.h * 0.56f;
    float xm = box.x + box.w * 0.45f;      float ym = box.y + box.h - pad;
    float x2 = box.x + box.w - pad;        float y2 = box.y + pad + 1.0f;

    const float cx = box.x + box.w * 0.5f;
    const float cy = box.y + box.h * 0.5f;
    auto S = [&](float& x, float& y){ x = cx + (x - cx) * scaleX; y = cy + (y - cy) * scaleY; };

    S(x1,y1); S(xm,ym); S(x2,y2);

    float t = thickness * std::min(scaleX, scaleY);
    UIHelpers::DrawRoundStrokeLine(r, x1, y1, xm, ym, t, color);
    UIHelpers::DrawRoundStrokeLine(r, xm, ym, x2, y2, t, color);
    UIHelpers::DrawFilledCircle(r, (int)std::round(xm), (int)std::round(ym),
                                (int)std::round(t * 0.50f), color);
}


static inline SDL_Color PrimaryBlue() { return SDL_Color{ 0x0D, 0x6E, 0xFD, 255 }; }
static inline SDL_Color Darken(SDL_Color c, int d){ return UIHelpers::Darken(c, d); }

UIRadioButton::UIRadioButton(const std::string& label, int x, int y, int w, int h, UIRadioGroup* group, int id, TTF_Font* font)
    : label(label), id(id), group(group), font(font)
{
    bounds = { x, y, w, h };
}

void UIRadioButton::setFont(TTF_Font* f) { font = f; }
int  UIRadioButton::getID() const { return id; }
bool UIRadioButton::isHovered() const { return hovered; }

void UIRadioButton::handleEvent(const SDL_Event& e) {
    if (!enabled) return;

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        SDL_Point p{ e.button.x, e.button.y };
        if (SDL_PointInRect(&p, &bounds)) {
            pressed = true;
            if (focusable) focused = true;
        } else if (focusable) {
            focused = false;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        SDL_Point p{ e.button.x, e.button.y };
        if (pressed && SDL_PointInRect(&p, &bounds)) {
            if (group) group->select(id);
        }
        pressed = false;
    }

    if (focusable && focused && e.type == SDL_KEYDOWN && enabled) {
        if (e.key.keysym.sym == SDLK_SPACE || e.key.keysym.sym == SDLK_RETURN) {
            if (group) group->select(id);
        } else if (e.key.keysym.sym == SDLK_ESCAPE) {
            focused = false;
        }
    }
}

void UIRadioButton::update(float) {
    if (!enabled) { hovered = false; pressed = false; return; }
    int mx, my; SDL_GetMouseState(&mx, &my);
    SDL_Point p{ mx, my };
    hovered = SDL_PointInRect(&p, &bounds);
}

void UIRadioButton::render(SDL_Renderer* renderer) {
    TTF_Font* activeFont = font ? font : getThemeFont(getTheme());
    if (!activeFont) return;

    const bool selected = (group && group->getSelectedID() == id);

    const int cx = bounds.x + 12;
    const int cy = bounds.y + bounds.h / 2;
    const int outerRadius     = 9;
    const int borderThickness = 2;
    const int donutThickness  = 5;

    if (focusable && focused) {
        SDL_Color halo = {0x0D, 0x6E, 0xFD, 100};
        UIHelpers::DrawCircleRing(renderer, cx, cy, outerRadius + 3, 3, halo);
    }

    if (selected) {
        SDL_Color blue = {0x0D, 0x6E, 0xFD, 255};
        if (pressed) blue = UIHelpers::AdjustBrightness(blue, -18);
        UIHelpers::DrawCircleRing(renderer, cx, cy, outerRadius, donutThickness, blue);
    } else {
        SDL_Color gray = {160,160,160,255};
        if (pressed) gray = UIHelpers::AdjustBrightness(gray, -18);
        UIHelpers::DrawCircleRing(renderer, cx, cy, outerRadius, borderThickness, gray);
    }

    SDL_Color textCol = {0, 0, 0,255};
    SDL_Surface* s = TTF_RenderText_Blended(activeFont, label.c_str(), textCol);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_Rect textRect = { bounds.x + 30, bounds.y + (bounds.h - (s->h))/2, s->w, s->h };
    SDL_RenderCopy(renderer, t, nullptr, &textRect);
    SDL_FreeSurface(s);
    SDL_DestroyTexture(t);
}


void UIRadioGroup::addButton(std::shared_ptr<UIRadioButton> btn) {
    buttons.push_back(btn);
}

void UIRadioGroup::select(int id) {
    selectedID = id;
}

int UIRadioGroup::getSelectedID() const {
    return selectedID;
}


UIButton::UIButton(const std::string& text, int x, int y, int w, int h, TTF_Font* f)
    : label(text), font(f)
{
    bounds = { x, y, w, h };
}

void UIButton::setOnClick(std::function<void()> callback) {
    onClick = callback;
}

void UIButton::setText(const std::string& newText) {
    label = newText;
}

const std::string& UIButton::getText() const {
    return label;
}

void UIButton::handleEvent(const SDL_Event& e) {
    if (!enabled) return;

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        if (mx >= bounds.x && mx <= bounds.x + bounds.w &&
            my >= bounds.y && my <= bounds.y + bounds.h) {
            pressed = true;
            if (focusable) focused = true;
        } else if (focusable) {
            focused = false;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && pressed) {
        int mx = e.button.x, my = e.button.y;
        if (mx >= bounds.x && mx <= bounds.x + bounds.w &&
            my >= bounds.y && my <= bounds.y + bounds.h) {
            if (onClick) onClick();
        }
        pressed = false;
    }

    if (focusable && focused && e.type == SDL_KEYDOWN && enabled) {
        if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) {
            pressed = true;
        }
    } else if (focusable && focused && e.type == SDL_KEYUP) {
        if ((e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) && pressed) {
            if (onClick) onClick();
            pressed = false;
        }
    }
}

void UIButton::update(float) {
    if (!enabled) { hovered = false; pressed = false; return; }
    int mx, my; SDL_GetMouseState(&mx, &my);
    SDL_Point mousePoint = { mx, my };
    hovered = SDL_PointInRect(&mousePoint, &bounds);
}


void UIButton::render(SDL_Renderer* renderer) {
    const UITheme& theme = getTheme();
    SDL_Color baseBg   = customBgColor   ? *customBgColor   : theme.backgroundColor;
    SDL_Color baseText = customTextColor ? *customTextColor : theme.textColor;

    Uint8 globalAlpha = enabled ? 255 : 128;

    SDL_Color bg = baseBg;
    if (enabled) {
        if (pressed)      bg = UIHelpers::Darken(baseBg, 10);
        else if (hovered) bg = UIHelpers::PickHoverColor(baseBg);
    }
    bg.a = globalAlpha;

    if (focusable && focused) {
        SDL_Color ring = UIHelpers::PickFocusRing(baseBg);
        ring.a = (Uint8)std::min<int>(178, globalAlpha);
        UIHelpers::StrokeRoundedRectOutside(renderer, bounds, cornerRadius, 2, ring, bg);
    }

    SDL_Rect dst = bounds;
    if (pressed) { dst.y += pressOffset; dst.x += 1; dst.w -= 2; dst.h -= 2; }

    UIHelpers::FillRoundedRect(renderer, dst.x, dst.y, dst.w, dst.h, cornerRadius, bg);

    TTF_Font* activeFont = font ? font : getThemeFont(getTheme());
    if (!activeFont) return;

    SDL_Color txt = baseText; txt.a = globalAlpha;
    SDL_Surface* s = TTF_RenderText_Blended(activeFont, label.c_str(), txt);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
    if (!t) { SDL_FreeSurface(s); return; }
    SDL_Rect r = { dst.x + (dst.w - s->w)/2, dst.y + (dst.h - s->h)/2, s->w, s->h };
    SDL_RenderCopy(renderer, t, nullptr, &r);
    SDL_FreeSurface(s);
    SDL_DestroyTexture(t);
}


void UIButton::setFont(TTF_Font* f) {
    font = f;
}

bool UIButton::isHovered() const {
    return hovered;
}

UIButton* UIButton::setTextColor(SDL_Color c) {
    customTextColor = c;
    return this;
}

UIButton* UIButton::setBackgroundColor(SDL_Color c) {
    customBgColor = c;
    return this;
}

UIButton* UIButton::setBorderColor(SDL_Color c) {
    customBorderColor = c;
    return this;
}


UILabel::UILabel(const std::string& text, int x, int y, int w, int h, TTF_Font* font)
    : text(text), font(font)
{
    bounds = { x, y, w, h };
}

void UILabel::render(SDL_Renderer* renderer) {
    TTF_Font* activeFont = font ? font : getThemeFont(getTheme());
    if (!activeFont) {
        SDL_Log("UILabel: No valid font to render text.");
        return;
    }

    const SDL_Color& textColor = (color.a == 0) ? getTheme().textColor : color;

    SDL_Surface* textSurface = TTF_RenderText_Blended(activeFont, text.c_str(), textColor);
    if (!textSurface) {
        SDL_Log("UILabel: Failed to render text surface: %s", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!texture) {
        SDL_Log("UILabel: Failed to create texture from surface: %s", SDL_GetError());
        SDL_FreeSurface(textSurface);
        return;
    }

    SDL_Rect dstRect = {
        bounds.x,
        bounds.y + (bounds.h - textSurface->h) / 2,
        textSurface->w,
        textSurface->h
    };

    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(texture);
}


UILabel* UILabel::setColor(SDL_Color newColor) {
    color = newColor;
    return this;
}

SDL_Color UILabel::getColor() const {
    return color;
}


UICheckbox::UICheckbox(const std::string& label, int x, int y, int w, int h, bool& bind, TTF_Font* f)
    : label(label), linkedValue(bind), font(f)
{
    bounds = { x, y, w, h };
}

void UICheckbox::setFont(TTF_Font* f) {
    font = f;
}

void UICheckbox::handleEvent(const SDL_Event& e) {
    if (!enabled) return;

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        if (mx >= bounds.x && mx <= bounds.x + bounds.w &&
            my >= bounds.y && my <= bounds.y + bounds.h) {
            linkedValue.get() = !linkedValue.get();
            if (focusable) focused = true;
        } else if (focusable) {
            focused = false;
        }
    }

    if (!focused || !enabled) return;

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_SPACE:
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                linkedValue.get() = !linkedValue.get();
                break;
            case SDLK_ESCAPE:
                focused = false;
                break;
            default: break;
        }
    }
}

bool UICheckbox::isHovered() const {
    return hovered;
}

void UICheckbox::update(float) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    SDL_Point mousePoint = { mx, my };
    hovered = SDL_PointInRect(&mousePoint, &bounds);
}

void UICheckbox::render(SDL_Renderer* renderer) {
    TTF_Font* activeFont = font ? font : UIConfig::getDefaultFont();
    if (!activeFont) return;

    const bool isChecked = linkedValue.get();
    const int  boxSize   = 18;
    const int  radius    = 4;

    SDL_Color textCol   = hasCustomTextColor ? customTextColor
                                             : UIHelpers::RGBA(33,37,41, enabled ? 255 : 160);
    SDL_Color borderCol = hasCustomBorderColor ? customBorderColor
                                               : (hovered ? UIHelpers::RGBA(120,120,120)
                                                          : UIHelpers::RGBA(160,160,160));
    SDL_Color fillCol   = hasCustomBoxBgColor ? customBoxBgColor
                                              : UIHelpers::RGBA(255,255,255);
    SDL_Color tickCol   = hasCustomCheckedColor ? customCheckedColor
                                                : UIHelpers::RGBA(13,110,253);

    const UITheme& theme = getTheme();
    SDL_Color parentBg   = theme.backgroundColor; // what’s behind the checkbox

    SDL_Rect box = { bounds.x, bounds.y + (bounds.h - boxSize)/2, boxSize, boxSize };

    int stroke = std::max(0, borderPx);
    int innerRadius = std::max(0, radius - stroke);
    SDL_Rect inner = { box.x + stroke, box.y + stroke, box.w - 2*stroke, box.h - 2*stroke };

    if (stroke > 0) {
        if (fillCol.a == 0) {
            // transparent center: ring over the parent's background (looks like a hole)
            UIHelpers::StrokeRoundedRectOutside(renderer, inner, innerRadius, stroke, borderCol, parentBg);
        } else {
            // non-transparent: first fill, then draw the ring using the same innerBg
            UIHelpers::FillRoundedRect(renderer, inner.x, inner.y, inner.w, inner.h, innerRadius, fillCol);
            UIHelpers::StrokeRoundedRectOutside(renderer, inner, innerRadius, stroke, borderCol, fillCol);
        }
    } else {
        // no border: just fill if any
        if (fillCol.a > 0)
            UIHelpers::FillRoundedRect(renderer, inner.x, inner.y, inner.w, inner.h, innerRadius, fillCol);
    }

    if (isChecked) {
        float pad   = 3.5f;
        float thick = std::clamp(box.w * 0.16f, 1.5f, 3.0f);
        SDL_Rect markBox = box; markBox.y -= 1;
        UIHelpers::DrawCheckmark(renderer, markBox, thick, tickCol, pad);
    }

    const int textLeft = box.x + box.w + 8;
    SDL_Surface* s = TTF_RenderText_Blended(activeFont, label.c_str(), textCol);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_Rect tr = { textLeft, bounds.y + (bounds.h - s->h)/2, s->w, s->h };
    SDL_RenderCopy(renderer, t, nullptr, &tr);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}


UITextField::UITextField(const std::string& label, int x, int y, int w, int h, std::string& bind, int maxLen)
    : label(label), linkedText(bind), maxLength(maxLen)
{
    bounds = { x, y, w, h };
    lastBlinkTicks = SDL_GetTicks();
}

UITextField* UITextField::setPlaceholder(const std::string& text) {
    placeholder = text;
    return this;
}

UITextField* UITextField::setFont(TTF_Font* f) {
    font = f;
    return this;
}

UITextField* UITextField::setInputType(InputType type) {
    inputType = type;
    return this;
}

bool UITextField::isHovered() const {
    return hovered;
}

void UITextField::handleEvent(const SDL_Event& e) {
    if (!enabled) return;

    auto inside = [&](int x, int y) {
        return (x >= bounds.x && x < bounds.x + bounds.w &&
                y >= bounds.y && y < bounds.y + bounds.h);
    };

    auto markTyping = [&](){
        lastInputTicks = SDL_GetTicks();
        cursorVisible  = true;
        lastBlinkTicks = lastInputTicks;
    };

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        const bool wasFocused = focused;
        focused = inside(e.button.x, e.button.y);
        if (focused) {
            if (!wasFocused) SDL_StartTextInput();
            markTyping();
        } else {
            if (wasFocused) SDL_StopTextInput();
            cursorVisible = false;
        }
        return;
    }

    if (!focused) return;

    if (e.type == SDL_TEXTINPUT) {
        std::string& s = linkedText.get();
        for (const char* p = e.text.text; *p; ++p) {
            if ((int)s.size() >= maxLength) break;
            unsigned char c = static_cast<unsigned char>(*p);
            if (c >= 32) s.push_back(char(c));
        }
        markTyping();
        return;
    }

    if (e.type == SDL_KEYDOWN) {
        std::string& s = linkedText.get();
        switch (e.key.keysym.sym) {
            case SDLK_BACKSPACE:
                if (!s.empty()) s.pop_back();
                markTyping();
                break;

            case SDLK_DELETE:
                if (!s.empty()) s.pop_back();
                markTyping();
                break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (onSubmit) onSubmit(s);
                markTyping();
                break;

            case SDLK_ESCAPE:
                focused = false;
                SDL_StopTextInput();
                cursorVisible = false;
                break;

            case SDLK_LEFT:
            case SDLK_RIGHT:
            case SDLK_HOME:
            case SDLK_END:
                markTyping();
                break;
        }
    }
}


void UITextField::update(float) {
    int mx, my; SDL_GetMouseState(&mx, &my);
    hovered = (mx >= bounds.x && mx < bounds.x + bounds.w &&
               my >= bounds.y && my < bounds.y + bounds.h);

    if (!enabled) { cursorVisible = false; return; }
    if (!focused) { cursorVisible = false; return; }

    const Uint32 now          = SDL_GetTicks();
    const Uint32 typingHoldMs = 300;
    const Uint32 blinkMs      = 530;

    if (now - lastInputTicks < typingHoldMs) {
        cursorVisible  = true;
        lastBlinkTicks = now;
    } else {
        if (now - lastBlinkTicks >= blinkMs) {
            cursorVisible  = !cursorVisible;
            lastBlinkTicks = now;
        }
    }
}


void UITextField::render(SDL_Renderer* renderer) {
    TTF_Font* activeFont = font ? font : UIConfig::getDefaultFont();
    if (!activeFont) return;

    SDL_Color baseBg      = {255,255,255,255};
    SDL_Color baseBorder  = {180,180,180,255};
    SDL_Color baseText    = {73, 80, 87,255};
    SDL_Color baseCursor  = {73, 80, 87,255};
    SDL_Color placeholderCol = {160,160,160,255};
    SDL_Color focusBlue   = {13,110,253,178};

    if (focused) {
        UIHelpers::StrokeRoundedRectOutside(renderer, bounds, cornerRadius, 2, focusBlue, baseBg);
    }

    SDL_Rect dst = bounds;

    if (borderPx > 0) {
        UIHelpers::FillRoundedRect(renderer, dst.x, dst.y, dst.w, dst.h, cornerRadius, baseBorder);
        SDL_Rect inner = { dst.x + borderPx, dst.y + borderPx, dst.w - 2*borderPx, dst.h - 2*borderPx };
        UIHelpers::FillRoundedRect(renderer, inner.x, inner.y, inner.w, inner.h, std::max(0, cornerRadius - borderPx), baseBg);
        dst = inner;
    } else {
        UIHelpers::FillRoundedRect(renderer, dst.x, dst.y, dst.w, dst.h, cornerRadius, baseBg);
    }

    std::string toRender = (inputType == InputType::PASSWORD)
        ? std::string(linkedText.get().size(), '*')
        : linkedText.get();

    SDL_Color drawCol = baseText;
    if (toRender.empty() && !focused && !placeholder.empty()) {
        toRender = placeholder;
        drawCol = placeholderCol;
    }

    int cursorX = dst.x + 8;
    int cursorH = TTF_FontHeight(activeFont);
    int cursorY = dst.y + (dst.h - cursorH) / 2;

    if (!toRender.empty()) {
        SDL_Surface* textSurface = TTF_RenderText_Blended(activeFont, toRender.c_str(), drawCol);
        if (textSurface) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect = {
                dst.x + 8,
                dst.y + (dst.h - textSurface->h) / 2,
                textSurface->w,
                textSurface->h
            };
            SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
            SDL_DestroyTexture(textTexture);

            cursorX = textRect.x + textRect.w;
            cursorH = textSurface->h;
            cursorY = textRect.y;

            SDL_FreeSurface(textSurface);
        }
    }

    if (focused && cursorVisible) {
        SDL_SetRenderDrawColor(renderer, baseCursor.r, baseCursor.g, baseCursor.b, baseCursor.a);
        SDL_Rect cursorRect = { cursorX, cursorY, 1, cursorH };
        SDL_RenderFillRect(renderer, &cursorRect);
    }
}





UIComboBox::UIComboBox(int x, int y, int w, int h, const std::vector<std::string>& options, int& selectedIndex)
    : options(options), selectedIndex(selectedIndex)
{
    bounds = { x, y, w, h };
}

void UIComboBox::setFont(TTF_Font* f) {
    font = f;
}

void UIComboBox::setOnSelect(std::function<void(int)> callback) {
    onSelect = callback;
}

bool UIComboBox::isHovered() const {
    return hovered;
}

bool UIComboBox::isExpanded() const {
    return expanded;

}

int UIComboBox::getItemCount() const {
    return static_cast<int>(options.size());
}

int UIComboBox::getItemHeight() const {
    return bounds.h;
}

const SDL_Rect& UIComboBox::getBounds() const {
    return bounds;
}

void UIComboBox::handleEvent(const SDL_Event& e) {
    if (!enabled) return;

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        SDL_Point p{ e.button.x, e.button.y };

        // Click on field → toggle, focus
        if (SDL_PointInRect(&p, &bounds)) {
            expanded = !expanded;
            if (focusable) focused = true;
            if (expanded) hoveredIndex = 0;          // first item active by default
            return;
        }

        // Click on list items
        if (expanded) {
            const int ih = bounds.h;
            for (int i = 0; i < (int)options.size(); ++i) {
                SDL_Rect itemRect{ bounds.x, bounds.y + (i + 1) * ih, bounds.w, ih };
                if (SDL_PointInRect(&p, &itemRect)) {
                    selectedIndex.get() = i;
                    if (onSelect) onSelect(i);
                    expanded = false;
                    return;
                }
            }
        }

        // Click outside → close and blur
        expanded = false;
        if (focusable) focused = false;
    }

    if (!focused || !enabled) return;

    if (e.type == SDL_KEYDOWN) {
        if (!expanded && (e.key.keysym.sym == SDLK_SPACE || e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_DOWN)) {
            expanded = true;
            hoveredIndex = 0;
        } else if (expanded) {
            if (e.key.keysym.sym == SDLK_DOWN) {
                hoveredIndex = std::min((int)options.size()-1, (hoveredIndex < 0 ? 0 : hoveredIndex + 1));
            } else if (e.key.keysym.sym == SDLK_UP) {
                hoveredIndex = std::max(0, hoveredIndex - 1);
            } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) {
                if (hoveredIndex >= 0) {
                    selectedIndex.get() = hoveredIndex;
                    if (onSelect) onSelect(hoveredIndex);
                }
                expanded = false;
            } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                expanded = false;
            }
        } else if (e.key.keysym.sym == SDLK_ESCAPE) {
            focused = false;
        }
    }
}


void UIComboBox::update(float) {
    int mx, my; SDL_GetMouseState(&mx, &my);
    SDL_Point p{ mx, my };
    hovered = SDL_PointInRect(&p, &bounds);

    if (!expanded) { return; }

    // Track hover in list; if mouse not over any item, keep current or default to first.
    hoveredIndex = -1;
    const int ih = bounds.h;
    for (int i = 0; i < (int)options.size(); ++i) {
        SDL_Rect itemRect{ bounds.x, bounds.y + (i + 1) * ih, bounds.w, ih };
        if (SDL_PointInRect(&p, &itemRect)) {
            hoveredIndex = i;
            break;
        }
    }
    if (hoveredIndex == -1) hoveredIndex = 0; // first stays active by default
}



void UIComboBox::render(SDL_Renderer* renderer) {
    const UITheme& theme = getTheme();
    TTF_Font* activeFont = font ? font : getThemeFont(theme);
    if (!activeFont) return;

    // Field colors (defaults)
    SDL_Color textCol    = UIHelpers::RGBA(33, 37, 41);   // #212529
    SDL_Color bgCol      = UIHelpers::RGBA(255,255,255);  // white
    SDL_Color borderCol  = UIHelpers::RGBA(180,180,180);  // default border
    SDL_Color arrowCol   = UIHelpers::RGBA(108,117,125);  // #6c757d
    SDL_Color focusRing  = UIHelpers::RGBA(13,110,253,178); // #0D6EFD @ ~70%
    const int r = cornerRadius;

    // Focus ring ONLY on the field
    if (focusable && focused) {
        UIHelpers::StrokeRoundedRectOutside(renderer, bounds, r, 2, focusRing, bgCol);
    }

    // Field: rounded, white, gray border
    if (true) { // draw border then inset fill to get a crisp 1px border
        UIHelpers::FillRoundedRect(renderer, bounds.x, bounds.y, bounds.w, bounds.h, r, borderCol);
        SDL_Rect inner{ bounds.x + 1, bounds.y + 1, bounds.w - 2, bounds.h - 2 };
        UIHelpers::FillRoundedRect(renderer, inner.x, inner.y, inner.w, inner.h, std::max(0, r - 1), bgCol);

        // Text
        const std::string selectedText = options.empty() ? "" : options[selectedIndex.get()];
        if (!selectedText.empty()) {
            SDL_Surface* s = TTF_RenderText_Blended(activeFont, selectedText.c_str(), textCol);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect tr{ inner.x + 10, inner.y + (inner.h - s->h)/2, s->w, s->h };
                SDL_RenderCopy(renderer, t, nullptr, &tr);
                SDL_DestroyTexture(t);
                SDL_FreeSurface(s);
            }
        }

        // Chevron (smooth V) at right using rounded strokes
        float cx = float(inner.x + inner.w - 16);
        float cy = float(inner.y + inner.h/2);
        float half = 5.0f;
        float thick = 2.2f;
        UIHelpers::DrawRoundStrokeLine(renderer, cx - half, cy - 2.5f, cx, cy + 2.5f, thick, arrowCol);
        UIHelpers::DrawRoundStrokeLine(renderer, cx, cy + 2.5f, cx + half, cy - 2.5f, thick, arrowCol);
    }

    // Dropdown list
    if (expanded && !options.empty()) {
        const int ih = bounds.h;
        SDL_Rect listRect{ bounds.x, bounds.y + bounds.h, bounds.w, ih * (int)options.size() };

        // Outer black border, no rounded corners, white fill, no internal borders
        SDL_Color listBorder = UIHelpers::RGBA(0,0,0);
        SDL_Color listBg     = UIHelpers::RGBA(255,255,255);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, listBg.r, listBg.g, listBg.b, listBg.a);
        SDL_RenderFillRect(renderer, &listRect);
        SDL_SetRenderDrawColor(renderer, listBorder.r, listBorder.g, listBorder.b, listBorder.a);
        SDL_RenderDrawRect(renderer, &listRect);

        // Items
        for (int i = 0; i < (int)options.size(); ++i) {
            SDL_Rect itemRect{ listRect.x, listRect.y + i*ih, listRect.w, ih };
            bool active = (i == hoveredIndex);
            SDL_Color bg = active ? UIHelpers::RGBA(13,110,253) : listBg; // #0D6EFD
            SDL_Color fg = active ? UIHelpers::RGBA(255,255,255) : textCol;

            if (active) {
                SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
                SDL_RenderFillRect(renderer, &itemRect);
            }
            SDL_Surface* s = TTF_RenderText_Blended(activeFont, options[i].c_str(), fg);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect tr{ itemRect.x + 10, itemRect.y + (itemRect.h - s->h)/2, s->w, s->h };
                SDL_RenderCopy(renderer, t, nullptr, &tr);
                SDL_DestroyTexture(t);
                SDL_FreeSurface(s);
            }
        }
    }
}

UIComboBox* UIComboBox::setTextColor(SDL_Color c) {
    customTextColor = c;
    return this;
}




UISlider::UISlider(const std::string& label, int x, int y, int w, int h, float& bind, float min, float max)
    : label(label), linkedValue(bind), minVal(min), maxVal(max)
{
    bounds = { x, y, w, h };
}

void UISlider::handleEvent(const SDL_Event& e) {
    if (!enabled) return;

    auto clamp01 = [](float v){ return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); };

    const int trackH = 6;
    SDL_Rect track = {
        bounds.x,
        bounds.y + (bounds.h - trackH)/2,
        bounds.w,
        trackH
    };

    float t = (linkedValue.get() - minVal) / (maxVal - minVal);
    t = clamp01(t);
    const int usable = track.w - 2*thumbRadius;
    const int cx = track.x + thumbRadius + int(std::round(t * usable));
    const int cy = bounds.y + bounds.h/2;

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        const int dx = mx - cx, dy = my - cy;
        const bool onThumb = (dx*dx + dy*dy) <= (thumbRadius*thumbRadius);
        const bool onTrack =
            (mx >= track.x && mx < track.x + track.w &&
             my >= track.y - thumbRadius && my < track.y + track.h + thumbRadius);

        if (onThumb || onTrack) {
            if (focusable) focused = true;
            dragging = true;

            float nt = clamp01( (float(mx) - (track.x + thumbRadius)) / float(usable) );
            linkedValue.get() = minVal + nt * (maxVal - minVal);
        } else if (focusable) {
            focused = false;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        dragging = false;
    } else if (e.type == SDL_MOUSEMOTION && dragging) {
        int mx = e.motion.x;
        float nt = clamp01( (float(mx) - (track.x + thumbRadius)) / float(usable) );
        linkedValue.get() = minVal + nt * (maxVal - minVal);
    }

    if (focused && e.type == SDL_KEYDOWN) {
        const float step = (maxVal - minVal) * 0.01f;
        if (e.key.keysym.sym == SDLK_LEFT)  linkedValue.get() = std::max(minVal, linkedValue.get() - step);
        if (e.key.keysym.sym == SDLK_RIGHT) linkedValue.get() = std::min(maxVal, linkedValue.get() + step);
        if (e.key.keysym.sym == SDLK_ESCAPE) focused = false;
    }
}


bool UISlider::isHovered() const {
    return hovered;
}

void UISlider::update(float) {
    int mx, my; SDL_GetMouseState(&mx, &my);
    hovered = (mx >= bounds.x && mx < bounds.x + bounds.w &&
               my >= bounds.y && my < bounds.y + bounds.h);
}


void UISlider::render(SDL_Renderer* renderer) {
    SDL_Color trackCol  = {224,224,224,255};
    SDL_Color thumbCol  = {0,123,255,255};
    SDL_Color thumbDrag = UIHelpers::AdjustBrightness(thumbCol, +18);
    SDL_Color ringCol   = {13,110,253,178};

    const int trackH = 6;
    SDL_Rect track = {
        bounds.x,
        bounds.y + (bounds.h - trackH)/2,
        bounds.w,
        trackH
    };

    UIHelpers::FillRoundedRect(renderer, track.x, track.y, track.w, track.h, trackH/2, trackCol);

    auto clamp01 = [](float v){ return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); };
    float t = (linkedValue.get() - minVal) / (maxVal - minVal);
    t = clamp01(t);
    const int usable = track.w - 2*thumbRadius;
    const int cx = track.x + thumbRadius + int(std::round(t * usable));
    const int cy = bounds.y + bounds.h/2;

    if (focusable && focused) {
        UIHelpers::DrawCircleRing(renderer, cx, cy, thumbRadius + 3, 3, ringCol);
    }

    SDL_Color drawThumb = dragging ? thumbDrag : thumbCol;
    UIHelpers::DrawFilledCircle(renderer, cx, cy, thumbRadius, drawThumb);
}


UISpinner::UISpinner(int x, int y, int w, int h, int& bind, int min, int max, int step)
    : value(bind), minValue(min), maxValue(max), step(step)
{
    bounds = { x, y, w, h };
}

void UISpinner::setFont(TTF_Font* f) {
    font = f;
}

void UISpinner::setOnChange(std::function<void(int)> callback) {
    onChange = callback;
}

bool UISpinner::isHovered() const {
    return hoveredMinus || hoveredPlus;
}

void UISpinner::handleEvent(const SDL_Event& e) {
    int mx = e.button.x;
    int my = e.button.y;
    SDL_Point point = { mx, my };

    SDL_Rect minusRect = { bounds.x, bounds.y, bounds.h, bounds.h };
    SDL_Rect plusRect = { bounds.x + bounds.w - bounds.h, bounds.y, bounds.h, bounds.h };

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        if (SDL_PointInRect(&point, &minusRect)) {
            if (value.get() > minValue) {
                value.get() -= step;
                if (onChange) onChange(value.get());
            }
            heldButton = HeldButton::DECREMENT;
            pressStartTime = SDL_GetTicks();
            lastStepTime = pressStartTime;
        } else if (SDL_PointInRect(&point, &plusRect)) {
            if (value.get() < maxValue) {
                value.get() += step;
                if (onChange) onChange(value.get());
            }
            heldButton = HeldButton::INCREMENT;
            pressStartTime = SDL_GetTicks();
            lastStepTime = pressStartTime;
        }
    }

    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        heldButton = HeldButton::NONE;
    }
}

void UISpinner::update(float) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    SDL_Point point = { mx, my };

    SDL_Rect minusRect = { bounds.x, bounds.y, bounds.h, bounds.h };
    SDL_Rect plusRect = { bounds.x + bounds.w - bounds.h, bounds.y, bounds.h, bounds.h };

    hoveredMinus = SDL_PointInRect(&point, &minusRect);
    hoveredPlus = SDL_PointInRect(&point, &plusRect);

    Uint32 now = SDL_GetTicks();
    if (heldButton != HeldButton::NONE && now - pressStartTime > 400 && now - lastStepTime > 100) {
        if (heldButton == HeldButton::INCREMENT && value.get() < maxValue) {
            value.get() += step;
            if (onChange) onChange(value.get());
        } else if (heldButton == HeldButton::DECREMENT && value.get() > minValue) {
            value.get() -= step;
            if (onChange) onChange(value.get());
        }
        lastStepTime = now;
    }
}

void UISpinner::render(SDL_Renderer* renderer) {
    const UITheme& theme = getTheme();
    TTF_Font* activeFont = font ? font : UIConfig::getDefaultFont();
    if (!activeFont) return;

    SDL_Rect minusRect = { bounds.x, bounds.y, bounds.h, bounds.h };
    SDL_Rect plusRect  = { bounds.x + bounds.w - bounds.h, bounds.y, bounds.h, bounds.h };
    SDL_Rect centerRect = {
        bounds.x + bounds.h,
        bounds.y,
        bounds.w - 2 * bounds.h,
        bounds.h
    };

    SDL_Color minusColor = hoveredMinus ? theme.hoverColor : theme.borderColor;
    SDL_Color plusColor  = hoveredPlus  ? theme.hoverColor : theme.borderColor;
    SDL_Color centerColor = theme.borderColor;

    SDL_SetRenderDrawColor(renderer, theme.backgroundColor.r, theme.backgroundColor.g, theme.backgroundColor.b, theme.backgroundColor.a);
    SDL_RenderFillRect(renderer, &bounds);

    SDL_SetRenderDrawColor(renderer, centerColor.r, centerColor.g, centerColor.b, centerColor.a);
    SDL_RenderDrawRect(renderer, &centerRect);

    SDL_SetRenderDrawColor(renderer, minusColor.r, minusColor.g, minusColor.b, 255);
    SDL_RenderDrawLine(renderer, minusRect.x, minusRect.y, minusRect.x + minusRect.w, minusRect.y);
    SDL_RenderDrawLine(renderer, minusRect.x, minusRect.y, minusRect.x, minusRect.y + minusRect.h);
    SDL_RenderDrawLine(renderer, minusRect.x, minusRect.y + minusRect.h - 1, minusRect.x + minusRect.w, minusRect.y + minusRect.h - 1);

    SDL_SetRenderDrawColor(renderer, plusColor.r, plusColor.g, plusColor.b, 255);
    SDL_RenderDrawLine(renderer, plusRect.x, plusRect.y, plusRect.x + plusRect.w, plusRect.y);
    SDL_RenderDrawLine(renderer, plusRect.x + plusRect.w - 1, plusRect.y, plusRect.x + plusRect.w - 1, plusRect.y + plusRect.h);
    SDL_RenderDrawLine(renderer, plusRect.x, plusRect.y + plusRect.h - 1, plusRect.x + plusRect.w, plusRect.y + plusRect.h - 1);

    SDL_SetRenderDrawColor(renderer, minusColor.r, minusColor.g, minusColor.b, 255);
    SDL_RenderDrawLine(renderer,
        minusRect.x + minusRect.w / 4,
        minusRect.y + minusRect.h / 2,
        minusRect.x + 3 * minusRect.w / 4,
        minusRect.y + minusRect.h / 2);

    SDL_SetRenderDrawColor(renderer, plusColor.r, plusColor.g, plusColor.b, 255);
    SDL_RenderDrawLine(renderer,
        plusRect.x + plusRect.w / 2,
        plusRect.y + plusRect.h / 4,
        plusRect.x + plusRect.w / 2,
        plusRect.y + 3 * plusRect.h / 4);
    SDL_RenderDrawLine(renderer,
        plusRect.x + plusRect.w / 4,
        plusRect.y + plusRect.h / 2,
        plusRect.x + 3 * plusRect.w / 4,
        plusRect.y + plusRect.h / 2);

    std::ostringstream oss;
    oss << value.get();
    SDL_Surface* surface = TTF_RenderText_Blended(activeFont, oss.str().c_str(), theme.textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect textRect = {
            centerRect.x + (centerRect.w - surface->w) / 2,
            centerRect.y + (centerRect.h - surface->h) / 2,
            surface->w,
            surface->h
        };
        SDL_RenderCopy(renderer, texture, nullptr, &textRect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}




UITextArea::UITextArea(const std::string& label, int x, int y, int w, int h, std::string& bind, int maxLen)
    : label(label), linkedText(bind), maxLength(maxLen)
{
    bounds = { x, y, w, h };
}

void UITextArea::setFont(TTF_Font* f) {
    font = f;
}

void UITextArea::setPlaceholder(const std::string& text) {
    placeholder = text;
}

void UITextArea::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        SDL_Point p{ e.button.x, e.button.y };
        bool wasFocused = focused;
        focused = SDL_PointInRect(&p, &bounds);
        if (!wasFocused && focused) {
            SDL_StartTextInput();
            cursorPos = linkedText.get().size();
            lastBlinkTime = SDL_GetTicks();
            cursorVisible = true;
        } else if (wasFocused && !focused) {
            SDL_StopTextInput();
        }
        SDL_Rect sb = getScrollbarRect();
        if (contentHeight > bounds.h) {
            float vr = float(bounds.h) / contentHeight;
            int th = std::max(int(bounds.h * vr), 20);
            int maxThumb = bounds.h - th;
            int ty = sb.y + int((scrollOffsetY / (contentHeight - bounds.h)) * maxThumb);
            SDL_Rect thumb{ sb.x, ty, sb.w, th };
            if (SDL_PointInRect(&p, &thumb)) {
                scrollbarDragging = true;
                scrollbarDragStartY = e.button.y;
                scrollbarThumbStartOffset = scrollOffsetY;
            }
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        scrollbarDragging = false;
    } else if (e.type == SDL_MOUSEMOTION) {
        if (scrollbarDragging) {
            int dy = e.motion.y - scrollbarDragStartY;
            scrollOffsetY = scrollbarThumbStartOffset + dy * (contentHeight - bounds.h) / bounds.h;
            scrollOffsetY = std::clamp(scrollOffsetY, 0.0f, contentHeight - float(bounds.h));
        }
        if (contentHeight > bounds.h) {
            SDL_Point p{ e.motion.x, e.motion.y };
            SDL_Rect sb = getScrollbarRect();
            float vr = float(bounds.h) / contentHeight;
            int th = std::max(int(bounds.h * vr), 20);
            int maxThumb = bounds.h - th;
            int ty = sb.y + int((scrollOffsetY / (contentHeight - bounds.h)) * maxThumb);
            SDL_Rect thumb{ sb.x, ty, sb.w, th };
            scrollbarHovered = SDL_PointInRect(&p, &thumb);
        } else {
            scrollbarHovered = false;
        }
    } else if (focused && e.type == SDL_TEXTINPUT) {
        std::string in = e.text.text;
        bool valid = true;
        switch (inputType) {
            case InputType::NUMERIC:
                valid = std::all_of(in.begin(), in.end(), ::isdigit);
                break;
            case InputType::EMAIL:
                valid = std::all_of(in.begin(), in.end(), [](char c){ return std::isalnum(c) || c=='@' || c=='.' || c=='-' || c=='_'; });
                break;
            default:
                break;
        }
        size_t room = maxLength > 0 ? maxLength - linkedText.get().size() : in.size();
        if (room > 0 && valid) {
            if (in.size() > room) in.resize(room);
            linkedText.get().insert(cursorPos, in);
            cursorPos += in.size();
        }
        updateCursorPosition();
    } else if (focused && e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_BACKSPACE && cursorPos > 0) {
            linkedText.get().erase(cursorPos - 1, 1);
            cursorPos--;
        } else if (e.key.keysym.sym == SDLK_RETURN) {
            if (linkedText.get().length() < size_t(maxLength)) {
                linkedText.get().insert(cursorPos, "\n");
                cursorPos++;
            }
        } else if (e.key.keysym.sym == SDLK_LEFT && cursorPos > 0) {
            cursorPos--;
        } else if (e.key.keysym.sym == SDLK_RIGHT && cursorPos < linkedText.get().size()) {
            cursorPos++;
        }
        lastBlinkTime = SDL_GetTicks();
        cursorVisible = true;
        updateCursorPosition();
    } else if (e.type == SDL_MOUSEWHEEL) {
        int mx, my; SDL_GetMouseState(&mx, &my);
        if (mx >= bounds.x && mx <= bounds.x + bounds.w && my >= bounds.y && my <= bounds.y + bounds.h) {
            int lh = TTF_FontHeight(font ? font : UIConfig::getDefaultFont());
            scrollOffsetY -= e.wheel.y * lh;
            scrollOffsetY = std::clamp(scrollOffsetY, 0.0f, contentHeight - float(bounds.h));
        }
    }
}


void UITextArea::update(float) {
    int mx, my; SDL_GetMouseState(&mx, &my);
    SDL_Point pt{ mx, my };
    hovered = SDL_PointInRect(&pt, &bounds);
    if (linkedText.get().length() > size_t(maxLength)) {
        linkedText.get().resize(maxLength);
        if (cursorPos > static_cast<size_t>(maxLength)) cursorPos = static_cast<size_t>(maxLength);
    }
    if (focused) {
        Uint32 now = SDL_GetTicks();
        if (now - lastBlinkTime >= 500) {
            cursorVisible = !cursorVisible;
            lastBlinkTime = now;
        }
    } else {
        cursorVisible = false;
        lastBlinkTime = SDL_GetTicks();
    }
    TTF_Font* fnt = font?font:UIConfig::getDefaultFont();
    auto lines = wrapTextToLines(linkedText.get().empty()?placeholder:linkedText.get(), fnt, bounds.w-10);
    contentHeight = float(lines.size()*TTF_FontHeight(fnt));
    scrollOffsetY = std::clamp(scrollOffsetY, 0.0f, std::max(0.0f, contentHeight - float(bounds.h)));
}



bool UITextArea::isHovered() const {
    return hovered;
}

int UITextArea::getWordCount() const {
    const std::string& text = linkedText.get();
    int count = 0;
    std::istringstream iss(text);
    std::string word;
    while (iss >> word) ++count;
    return count;
}

std::vector<std::string> UITextArea::wrapTextToLines(const std::string& text, TTF_Font* font, int maxWidth) {
    std::vector<std::string> lines;
    if (!font) return lines;
    std::string currentLine, currentWord;
    for (char c : text) {
        if (c == '\n') {
            lines.push_back(currentLine + currentWord);
            currentLine.clear();
            currentWord.clear();
            continue;
        }
        currentWord += c;
        std::string temp = currentLine + currentWord;
        int w, h;
        TTF_SizeText(font, temp.c_str(), &w, &h);
        if (w > maxWidth) {
            if (currentLine.empty()) {
                std::string part;
                for (size_t i = 0; i < currentWord.size(); ++i) {
                    std::string test = currentWord.substr(0, i+1);
                    TTF_SizeText(font, test.c_str(), &w, &h);
                    if (w > maxWidth) break;
                    part = test;
                }
                if (part.empty()) part = currentWord.substr(0,1);
                lines.push_back(part);
                currentWord = currentWord.substr(part.size());
            } else {
                lines.push_back(currentLine);
                currentLine = currentWord;
                currentWord.clear();
            }
        }
        if (c == ' ') {
            currentLine += currentWord;
            currentWord.clear();
        }
    }
    lines.push_back(currentLine + currentWord);
    return lines;
}

void UITextArea::render(SDL_Renderer* renderer) {
    const UITheme& theme = getTheme();
    TTF_Font* fnt = font ? font : UIConfig::getDefaultFont();
    if (!fnt) return;

    if (!label.empty()) {
        SDL_Surface* ls = TTF_RenderText_Blended(fnt, label.c_str(), theme.textColor);
        SDL_Texture* lt = SDL_CreateTextureFromSurface(renderer, ls);
        SDL_Rect labelRect = {bounds.x, bounds.y - ls->h - 4, ls->w, ls->h};
        SDL_RenderCopy(renderer, lt, nullptr, &labelRect);
        SDL_FreeSurface(ls);
        SDL_DestroyTexture(lt);
    }

    SDL_SetRenderDrawColor(renderer, theme.backgroundColor.r, theme.backgroundColor.g, theme.backgroundColor.b, theme.backgroundColor.a);
    SDL_RenderFillRect(renderer, &bounds);
    SDL_SetRenderDrawColor(renderer, theme.borderColor.r, theme.borderColor.g, theme.borderColor.b, theme.borderColor.a);
    SDL_RenderDrawRect(renderer, &bounds);

    SDL_RenderSetClipRect(renderer, &bounds);
    std::string txt = linkedText.get().empty() ? placeholder : linkedText.get();
    SDL_Color col = linkedText.get().empty() ? theme.placeholderColor : theme.textColor;
    auto lines = wrapTextToLines(txt, fnt, bounds.w - 10);
    int lh = TTF_FontHeight(fnt);
    int y = bounds.y + 5 - static_cast<int>(scrollOffsetY);

    for (const auto& line : lines) {
        if (!line.empty()) {
            SDL_Surface* s = TTF_RenderText_Blended(fnt, line.c_str(), col);
            SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
            SDL_Rect dst = {bounds.x + 5, y, s->w, s->h};
            SDL_RenderCopy(renderer, t, nullptr, &dst);
            SDL_FreeSurface(s);
            SDL_DestroyTexture(t);
        }
        y += lh;
    }

    if (focused && cursorVisible) {
        int onScreenY = cursorY - static_cast<int>(scrollOffsetY);
        onScreenY = std::clamp(onScreenY, bounds.y, bounds.y + bounds.h - lh);
        SDL_SetRenderDrawColor(renderer, theme.cursorColor.r, theme.cursorColor.g, theme.cursorColor.b, theme.cursorColor.a);
        SDL_Rect cursorRect = {cursorX, onScreenY, 2, lh};
        SDL_RenderFillRect(renderer, &cursorRect);
    }

    if (focused || !linkedText.get().empty()) {
        int words = getWordCount();
        std::string wcLabel = std::to_string(words) + " words";
        SDL_Surface* wcSurface = TTF_RenderText_Blended(fnt, wcLabel.c_str(), theme.placeholderColor);
        if (wcSurface) {
            SDL_Texture* wcTexture = SDL_CreateTextureFromSurface(renderer, wcSurface);
            SDL_Rect wcRect = {
                bounds.x,
                bounds.y + bounds.h + 4,
                wcSurface->w,
                wcSurface->h
            };
            SDL_RenderCopy(renderer, wcTexture, nullptr, &wcRect);
            SDL_FreeSurface(wcSurface);
            SDL_DestroyTexture(wcTexture);
        }
    }

    SDL_RenderSetClipRect(renderer, nullptr);
    if (contentHeight > bounds.h) {
        renderScrollbar(renderer);
    }
}


void UITextArea::updateCursorPosition() {
    TTF_Font* fnt = font ? font : UIConfig::getDefaultFont();
    int lh = TTF_FontHeight(fnt);
    std::string before = linkedText.get().substr(0, cursorPos);
    auto lines = wrapTextToLines(before, fnt, bounds.w - 10);
    int caretLine = int(lines.size()) - 1;
    const std::string& last = lines.empty() ? "" : lines.back();
    int w = 0;
    if (!last.empty()) {
        TTF_SizeText(fnt, last.c_str(), &w, nullptr);
    }
    cursorX = bounds.x + 5 + w;
    cursorY = bounds.y + 5 + caretLine * lh;
    float top = float(cursorY - bounds.y);
    float bottom = top + lh;
    if (top < scrollOffsetY) {
        scrollOffsetY = top;
    } else if (bottom > scrollOffsetY + bounds.h) {
        scrollOffsetY = bottom - bounds.h;
    }
    scrollOffsetY = std::clamp(scrollOffsetY, 0.0f, std::max(0.0f, contentHeight - float(bounds.h)));
}

SDL_Rect UITextArea::getScrollbarRect() const {
    const int W = 10;
    return { bounds.x + bounds.w - W - 2, bounds.y + 2, W, bounds.h - 4 };
}

void UITextArea::renderScrollbar(SDL_Renderer* renderer) {
    const UITheme& theme = getTheme();
    SDL_Rect sb = getScrollbarRect();
    float vr = float(bounds.h)/contentHeight;
    int th = std::max(int(bounds.h*vr), 20);
    float maxScroll = contentHeight - bounds.h;
    int maxThumb = bounds.h - th;
    int ty = sb.y + (maxScroll>0 ? int(scrollOffsetY/maxScroll*maxThumb) : 0);
    SDL_SetRenderDrawColor(renderer, theme.sliderTrackColor.r,theme.sliderTrackColor.g,theme.sliderTrackColor.b,150);
    SDL_RenderFillRect(renderer, &sb);
    SDL_Rect thumb{ sb.x, ty, sb.w, th };
    SDL_SetRenderDrawColor(renderer, theme.sliderThumbColor.r,theme.sliderThumbColor.g,theme.sliderThumbColor.b,200);
    SDL_RenderFillRect(renderer, &thumb);
}

bool UITextArea::isScrollbarHovered() const {
    return scrollbarHovered;
}

bool UITextArea::isScrollbarDragging() const {
    return scrollbarDragging;
}


UIGroupBox::UIGroupBox(const std::string& title, int x, int y, int w, int h)
    : title(title)
{
    bounds = { x, y, w, h };
    font = getThemeFont(getTheme());
}

void UIGroupBox::addChild(std::shared_ptr<UIElement> child) {
    children.push_back(child);
}

const std::vector<std::shared_ptr<UIElement>>& UIGroupBox::getChildren() const {
    return children;
}

void UIGroupBox::handleEvent(const SDL_Event& e) {
    for (auto& child : children)
        child->handleEvent(e);
}

void UIGroupBox::update(float dt) {
    for (auto& child : children)
        child->update(dt);
}

void UIGroupBox::render(SDL_Renderer* renderer) {
    const UITheme& theme = getTheme();
    SDL_SetRenderDrawColor(renderer,
        theme.borderColor.r,
        theme.borderColor.g,
        theme.borderColor.b,
        theme.borderColor.a);

    int titleWidth = 0, titleHeight = 0;
    if (!title.empty() && font) {
        TTF_SizeText(font, title.c_str(), &titleWidth, &titleHeight);
    }

    const int padding = 8;
    const int gapStart = bounds.x + padding;
    const int gapEnd = gapStart + titleWidth + padding;

    SDL_RenderDrawLine(renderer, bounds.x, bounds.y, gapStart, bounds.y);
    SDL_RenderDrawLine(renderer, gapEnd, bounds.y, bounds.x + bounds.w, bounds.y);

    SDL_RenderDrawLine(renderer, bounds.x, bounds.y, bounds.x, bounds.y + bounds.h);
    SDL_RenderDrawLine(renderer, bounds.x + bounds.w, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h);
    SDL_RenderDrawLine(renderer, bounds.x, bounds.y + bounds.h, bounds.x + bounds.w, bounds.y + bounds.h);

    if (!title.empty() && font) {
        SDL_Surface* surf = TTF_RenderText_Blended(font, title.c_str(), theme.textColor);
        if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect textRect = {
                bounds.x + padding + 4,
                bounds.y - surf->h / 2,
                surf->w,
                surf->h
            };
            SDL_RenderCopy(renderer, tex, nullptr, &textRect);
            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);
        }
    }

    for (auto& child : children) {
        child->render(renderer);
    }
}


namespace FormUI {

std::shared_ptr<UILabel> Layout::addLabel(const std::string& text, int width, int height) {
    auto labelEl = FormUI::Label(text, currentX, currentY, width, height, defaultFont);
    currentY += height + spacing;
    return labelEl;
}

std::shared_ptr<UICheckbox> Layout::addCheckbox(const std::string& label, bool& value, int width, int height) {
    auto checkbox = FormUI::Checkbox(label, currentX, currentY, width, height, value, defaultFont);
    currentY += height + spacing;
    return checkbox;
}

std::shared_ptr<UISlider> Layout::addSlider(const std::string& label, float& value, float min, float max, int width, int height) {
    auto slider = FormUI::Slider(label, currentX, currentY, width, height, value, min, max);
    currentY += height + spacing;
    return slider;
}

std::shared_ptr<UITextField> Layout::addTextField(const std::string& label, std::string& bind, int maxLen, int width, int height) {
    auto textField = FormUI::TextField(label, currentX, currentY, width, height, bind, maxLen);
    textField->setFont(defaultFont);
    currentY += height + spacing;
    return textField;
}

std::shared_ptr<UIButton> Layout::addButton(const std::string& label, std::function<void()> onClick, int width, int height, TTF_Font* font)
{
    auto button = FormUI::Button(label, currentX, currentY, width, height, onClick, font ? font : defaultFont);
    currentY += height + spacing;
    return button;
}

std::pair<std::shared_ptr<UILabel>, std::shared_ptr<UIButton>> Layout::addLabelButtonRow(
    const std::string& labelText,
    const std::string& buttonText,
    std::function<void()> onClick,
    int labelWidth,
    int buttonWidth,
    int height,
    TTF_Font* labelFont,
    TTF_Font* buttonFont
) {
    auto label = FormUI::Label(labelText, currentX, currentY, labelWidth, height, labelFont ? labelFont : defaultFont);
    auto button = FormUI::Button(buttonText, currentX + labelWidth + 10, currentY, buttonWidth, height, onClick, buttonFont ? buttonFont : defaultFont);
    currentY += height + spacing;
    return { label, button };
}

std::shared_ptr<UIRadioGroup> Layout::addRadioGroup(
    const std::vector<std::pair<std::string, int>>& options,
    int& selectedID,
    int width,
    int height,
    int groupSpacing
) {
    auto group = std::make_shared<UIRadioGroup>();
    group->select(selectedID);

    for (const auto& [label, id] : options) {
        auto btn = std::make_shared<UIRadioButton>(label, currentX, currentY, width, height, group.get(), id, defaultFont);
        group->addButton(btn);
        FormUI::AddElement(btn);
        currentY += height + groupSpacing;
    }

    return group;
}

std::shared_ptr<UIComboBox> Layout::addComboBox(const std::vector<std::string>& options, int& selectedIndex, int width, int height) {
    auto combo = FormUI::ComboBox(options, currentX, currentY, width, height, selectedIndex, defaultFont);
    currentY += height + spacing;
    return combo;
}

std::shared_ptr<UISpinner> Layout::addSpinner(int& bind, int min, int max, int step, int width, int height) {
    auto spinner = FormUI::Spinner(currentX, currentY, width, height, bind, min, max, step, defaultFont);
    currentY += height + spacing;
    return spinner;
}

std::shared_ptr<UITextArea> Layout::addTextArea(const std::string& label, std::string& bind, int maxLen, int width, int height) {
    auto textArea = FormUI::TextArea(label, currentX, currentY, width, height, bind, maxLen);
    textArea->setFont(defaultFont);
    currentY += height + spacing;
    return textArea;
}

}



void UIManager::addElement(std::shared_ptr<UIElement> el) {
    elements.push_back(el);
}

void UIManager::showPopup(std::shared_ptr<UIPopup> popup) {
    activePopup = popup;
}

void UIManager::closePopup() {
    activePopup = nullptr;
}

void UIManager::checkCursorForElement(const std::shared_ptr<UIElement>& el, SDL_Cursor*& cursorToUse) {
    if (!el->visible) return;

    if (auto ta = dynamic_cast<UITextArea*>(el.get())) {
        if (ta->isScrollbarHovered() || ta->isScrollbarDragging()) return;
        if (ta->isHovered()) { cursorToUse = ibeamCursor; return; }
    }

    if (el->isHovered()) {
        if (dynamic_cast<UITextField*>(el.get())) {
            cursorToUse = ibeamCursor;
            return;
        }

        if (cursorToUse != ibeamCursor &&
            (dynamic_cast<UIButton*>(el.get()) ||
             dynamic_cast<UICheckbox*>(el.get()) ||
             dynamic_cast<UISlider*>(el.get()) ||
             dynamic_cast<UIComboBox*>(el.get()) ||
             dynamic_cast<UISpinner*>(el.get()))) {
            cursorToUse = handCursor;
        }
    }

    if (auto group = dynamic_cast<UIGroupBox*>(el.get())) {
        for (auto& child : group->getChildren()) {
            checkCursorForElement(child, cursorToUse);
            if (cursorToUse == ibeamCursor) return;
        }
    }

    if (auto combo = dynamic_cast<UIComboBox*>(el.get())) {
        if (combo->isExpanded()) {
            int mx, my; SDL_GetMouseState(&mx, &my);
            int itemHeight = combo->getItemHeight();
            int baseY = combo->getBounds().y;
            int itemCount = combo->getItemCount();
            for (int i = 0; i < itemCount; ++i) {
                SDL_Rect itemRect = {
                    combo->getBounds().x,
                    baseY + (i + 1) * itemHeight,
                    combo->getBounds().w,
                    itemHeight
                };
                SDL_Point pt{ mx, my };
                if (SDL_PointInRect(&pt, &itemRect)) {
                    if (cursorToUse != ibeamCursor) cursorToUse = handCursor;
                    return;
                }
            }
        }
    }
}



void UIManager::handleEvent(const SDL_Event& e) {
    if (activePopup && activePopup->visible) {
        activePopup->handleEvent(e);
        return;
    }

    for (const auto& el : elements) {
        if (el->visible) {
            auto combo = dynamic_cast<UIComboBox*>(el.get());
            if (combo && combo->isExpanded()) {
                combo->handleEvent(e);
                return;
            }
        }
    }

    for (const auto& el : elements) {
        if (el->visible)
            el->handleEvent(e);
    }
}

void UIManager::update(float dt) {
    if (activePopup && !activePopup->visible) {
        activePopup = nullptr;
    }

    SDL_Cursor* cursorToUse = arrowCursor;

    if (activePopup && activePopup->visible) {
        activePopup->update(dt);
        for (const auto& child : activePopup->children) {
            checkCursorForElement(child, cursorToUse);
        }
    } else {
        for (const auto& el : elements) {
            auto combo = dynamic_cast<UIComboBox*>(el.get());
            if (combo && combo->isExpanded()) {
                combo->update(dt);
                checkCursorForElement(el, cursorToUse);
                if (SDL_GetCursor() != cursorToUse)
                    SDL_SetCursor(cursorToUse);
                return;
            }
        }
        for (const auto& el : elements) {
            el->update(dt);
            checkCursorForElement(el, cursorToUse);
        }
    }
    if (SDL_GetCursor() != cursorToUse)
        SDL_SetCursor(cursorToUse);
}


void UIManager::render(SDL_Renderer* renderer) {
    for (auto& el : elements) {
        if (el->visible)
            el->render(renderer);
    }
    if (activePopup && activePopup->visible) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
        SDL_Rect fullscreen = { 0, 0, 800, 600 };
        SDL_RenderFillRect(renderer, &fullscreen);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        activePopup->render(renderer);
    }
}
void UIManager::initCursors() {
    arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    ibeamCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    SDL_SetCursor(arrowCursor);
}

void UIManager::cleanupCursors() {
    SDL_FreeCursor(arrowCursor);
    SDL_FreeCursor(handCursor);
    SDL_FreeCursor(ibeamCursor);
}


namespace FormUI {
    static UIManager uiManager;
    static std::shared_ptr<UIPopup> internalPopup;

    void Init(TTF_Font* defaultFont) {
        if (defaultFont) {
            UIConfig::setDefaultFont(defaultFont);
        }
        uiManager.initCursors();
    }

    void Shutdown() {
        uiManager.cleanupCursors();
    }

    std::shared_ptr<UIButton> Button(const std::string& label, int x, int y, int w, int h, std::function<void()> onClick, TTF_Font* font)
    {
        auto btn = std::make_shared<UIButton>(label, x, y, w, h, font);
        if (onClick) btn->setOnClick(onClick);
        uiManager.addElement(btn);
        return btn;
    }

    std::shared_ptr<UICheckbox> Checkbox(const std::string& label, int x, int y, int w, int h, bool& bind, TTF_Font* font) {
        auto box = std::make_shared<UICheckbox>(label, x, y, w, h, bind, font);
        uiManager.addElement(box);
        return box;
    }

    std::shared_ptr<UILabel> Label(const std::string& text, int x, int y, int w, int h, TTF_Font* font) {
        auto label = std::make_shared<UILabel>(text, x, y, w, h, font);
        uiManager.addElement(label);
        return label;
    }

    std::shared_ptr<UISlider> Slider(const std::string& label, int x, int y, int w, int h, float& bind, float min, float max) {
        auto slider = std::make_shared<UISlider>(label, x, y, w, h, bind, min, max);
        uiManager.addElement(slider);
        return slider;
    }

    std::shared_ptr<UITextField> TextField(const std::string& label, int x, int y, int w, int h, std::string& bind, int maxLen) {
        auto field = std::make_shared<UITextField>(label, x, y, w, h, bind, maxLen);
        uiManager.addElement(field);
        return field;
    }

    std::shared_ptr<UIComboBox> ComboBox(const std::vector<std::string>& options, int x, int y, int w, int h, int& selectedIndex, TTF_Font* font) {
        auto box = std::make_shared<UIComboBox>(x, y, w, h, options, selectedIndex);
        box->setFont(font ? font : UIConfig::getDefaultFont());
        uiManager.addElement(box);
        return box;
    }

    std::shared_ptr<UISpinner> Spinner(int x, int y, int w, int h, int& bind, int min, int max, int step, TTF_Font* font) {
        auto spinner = std::make_shared<UISpinner>(x, y, w, h, bind, min, max, step);
        if (font) spinner->setFont(font);
        AddElement(spinner);
        return spinner;
    }

    std::shared_ptr<UITextArea> TextArea(const std::string& label, int x, int y, int w, int h, std::string& bind, int maxLen) {
        return std::make_shared<UITextArea>(label, x, y, w, h, bind, maxLen);
    }


    void AddElement(std::shared_ptr<UIElement> element) {
        uiManager.addElement(element);
    }

    void ShowPopup(std::shared_ptr<UIPopup> popup) {
        internalPopup = popup;
        uiManager.showPopup(popup);
    }

    void ClosePopup() {
        if (internalPopup) internalPopup->visible = false;
    }

    void HandleEvent(const SDL_Event& e) {
        uiManager.handleEvent(e);
    }

    void Update() {
        uiManager.update(0.0f);
    }

    void Render(SDL_Renderer* renderer) {
        uiManager.render(renderer);
    }
}

#endif // SDLFORMUI_HPP
#endif // SDLFORMUI_HPP