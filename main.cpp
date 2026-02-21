#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfx.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

const int WINDOW_WIDTH = 1000, WINDOW_HEIGHT = 600;
const int MENU_H = 48;
struct SpriteData {
    string name = "Mario";
    string xStr = "810", yStr = "100", sizeStr = "100", angleStr = "90";
    int x = 810, y = 100, size = 100, angle = 90;
} marioData;

// مشخص کردن اینکه کدام فیلد در حال ویرایش است (0: هیچکدام، 1: X، 2: Y، 3: Size، 4: Angle)
int activeField = 0;

// فیلدهای مستطیلی برای کلیک کاربر
SDL_Rect xInputRect = {760, 350, 60, 25};
SDL_Rect yInputRect = {860, 350, 60, 25};
SDL_Rect sizeInputRect = {760, 390, 60, 25};
SDL_Rect angleInputRect = {860, 390, 60, 25};
// ناحیه‌ها
SDL_Rect leftPanel = {80, 50, 200, 550};
SDL_Rect workspace = {280, 50, 470, 550};
SDL_Rect stagePanel = {710, 50, 290, 550};

//تعریف محدوده مربع بالایی برای ماریو
SDL_Rect spriteArea = {710, 50, 290, 275};
SDL_Texture *marioTexture = NULL;

// متغیر ذخیره عکس
SDL_Rect marioRect = {810, 100, 90, 90};

// محلی که شبه بلوک ها ظاهر می شوند
SDL_Rect subBlockArea = {90, 60, 180, 200};

// دکمه افزودن اکستنشن
SDL_Rect addExtensionBtn = {0, WINDOW_HEIGHT-40, 120, 40};

// منوی اکستنشن
bool extensionMenuOpen = false;
SDL_Rect getBackBtn = {10, 40, 120, 50};
SDL_Rect penBtn = {300, 50, 200, 200};

void updateMarioFromInputs() {
    try {
        if (!marioData.xStr.empty()) marioData.x = stoi(marioData.xStr);
        if (!marioData.yStr.empty()) marioData.y = stoi(marioData.yStr);
        if (!marioData.sizeStr.empty()) marioData.size = stoi(marioData.sizeStr);
        if (!marioData.angleStr.empty()) marioData.angle = stoi(marioData.angleStr);

        marioRect.x = marioData.x;
        marioRect.y = marioData.y;
        float scale = marioData.size / 100.0f;
        marioRect.w = 90 * scale;
        marioRect.h = 90 * scale;
    } catch (...) {}
}

void drawRect(SDL_Renderer *ren, SDL_Rect r, SDL_Color fill) {
    SDL_SetRenderDrawColor(ren, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(ren, &r);
}

bool pointInRect(int x, int y, const SDL_Rect &r) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}


struct BlockType {
    SDL_Color color;
    SDL_Rect protoRect;
    string name;
};

struct SubBlock {
    SDL_Color color;
    SDL_Rect rect;
    string name;
    int typeIndex;
};

struct BlockInstance {
    int typeIndex;
    SDL_Rect rect;
    int lastValidX, lastValidY;
    bool dragging = false;
    int dragStartX = 0, dragStartY = 0;
    string customName;
};

// شبه بلوک های قابل درگ
vector<BlockInstance> instances;
vector<SubBlock> currentSubBlocks;

// تابع ساخت شبه بلوک ها
void createSubBlocks(const BlockType &blockType) {
    currentSubBlocks.clear();
    int startX = subBlockArea.x + 10, startY = subBlockArea.y + 10;

    if (blockType.name == "motion") {
        string names[5] = {"move 10", "turn 15", "go to", "glide", "change x"};
        for (int i = 0; i < 5; i++) {
            SubBlock sb;
            sb.color = blockType.color;
            sb.rect = {startX, startY + (i * 45), 160, 35};
            sb.name = names[i];
            sb.typeIndex = 0;
            currentSubBlocks.push_back(sb);
        }
    } else if (blockType.name == "looks") {
        string names[5] = {"say", "think", "show", "hide", "change size"};
        for (int i = 0; i < 5; i++) {
            SubBlock sb;
            sb.color = blockType.color;
            sb.rect = {startX, startY + (i * 45), 160, 35};
            sb.name = names[i];
            sb.typeIndex = 1;
            currentSubBlocks.push_back(sb);
        }
    } else if (blockType.name == "sound") {
        string names[5] = {"play", "stop", "change vol", "set vol", "drum"};
        for (int i = 0; i < 5; i++) {
            SubBlock sb;
            sb.color = blockType.color;
            sb.rect = {startX, startY + (i * 45), 160, 35};
            sb.name = names[i];
            sb.typeIndex = 2;
            currentSubBlocks.push_back(sb);
        }
    } else if (blockType.name == "events") {
        string names[5] = {"when flag", "when key", "when click", "broadcast", "when loud"};
        for (int i = 0; i < 5; i++) {
            SubBlock sb;
            sb.color = blockType.color;
            sb.rect = {startX, startY + (i * 45), 160, 35};
            sb.name = names[i];
            sb.typeIndex = 3;
            currentSubBlocks.push_back(sb);
        }
    } else if (blockType.name == "control") {
        string names[5] = {"wait", "repeat", "forever", "if then", "if else"};
        for (int i = 0; i < 5; i++) {
            SubBlock sb;
            sb.color = blockType.color;
            sb.rect = {startX, startY + (i * 45), 160, 35};
            sb.name = names[i];
            sb.typeIndex = 4;
            currentSubBlocks.push_back(sb);
        }
    } else if (blockType.name == "senses") {
        string names[5] = {"touching", "distance", "ask", "key pressed", "mouse down"};
        for (int i = 0; i < 5; i++) {
            SubBlock sb;
            sb.color = blockType.color;
            sb.rect = {startX, startY + (i * 45), 160, 35};
            sb.name = names[i];
            sb.typeIndex = 5;
            currentSubBlocks.push_back(sb);
        }
    } else if (blockType.name == "operators") {
        string names[5] = {"add", "subtract", "multiply", "divide", "random"};
        for (int i = 0; i < 5; i++) {
            SubBlock sb;
            sb.color = blockType.color;
            sb.rect = {startX, startY + (i * 45), 160, 35};
            sb.name = names[i];
            sb.typeIndex = 6;
            currentSubBlocks.push_back(sb);
        }
    } else if (blockType.name == "variables") {
        string names[5] = {"set var", "change var", "show var", "hide var", "var"};
        for (int i = 0; i < 5; i++) {
            SubBlock sb;
            sb.color = blockType.color;
            sb.rect = {startX, startY + (i * 45), 160, 35};
            sb.name = names[i];
            sb.typeIndex = 7;
            currentSubBlocks.push_back(sb);
        }
    } else if (blockType.name == "pen") {
        string names[5] = {"pen down", "pen up", "set color", "set size", "stamp"};
        for (int i = 0; i < 5; i++) {
            SubBlock sb;
            sb.color = blockType.color;
            sb.rect = {startX, startY + (i * 45), 160, 35};
            sb.name = names[i];
            sb.typeIndex = 8; // New index for pen
            currentSubBlocks.push_back(sb);
        }
    }
}

void writeCenteredText(SDL_Renderer *ren, TTF_Font *font, const string& text, SDL_Rect rect) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), white);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    int tw, th;
    SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
    SDL_Rect dst = {rect.x + (rect.w - tw) / 2, rect.y + (rect.h - th) / 2, tw, th};
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

void writeLeftText(SDL_Renderer *ren, TTF_Font *font, const string& text, SDL_Rect rect, SDL_Color color, int pad) {
    SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), color);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    int tw, th;
    SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
    SDL_Rect dst = {rect.x + pad, rect.y + (rect.h - th) / 2, tw, th};
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

string getProjectPath() {
    char *base = SDL_GetBasePath();
    string path;
    if (base) {
        path = string(base) + "project.txt";
        SDL_free(base);
    } else
        path = "project.txt";
    return path;
}

bool saveBlocks(const string &path, const vector<BlockInstance> &instances) {
    ofstream out(path);
    if (!out) return false;
    out << instances.size() << "\n";
    for (auto &b: instances) {
        out << b.typeIndex << " "
            << b.rect.x << " " << b.rect.y << " "
            << b.rect.w << " " << b.rect.h << " "
            << b.customName << "\n";
    }
    return true;
}

bool loadBlocks(const string &path, vector<BlockInstance> &instances) {
    ifstream in(path);
    if (!in) return false;
    size_t n = 0;
    in >> n;
    instances.clear();
    for (size_t i = 0; i < n; ++i) {
        BlockInstance b;
        in >> b.typeIndex >> b.rect.x >> b.rect.y >> b.rect.w >> b.rect.h;
        in.ignore();
        getline(in, b.customName);
        b.lastValidX = b.rect.x;
        b.lastValidY = b.rect.y;
        b.dragging = false;
        instances.push_back(b);
    }
    return true;
}

// دیالوگ پرسش برای ذخیره قبل از New
int confirmSaveBeforeNew(SDL_Window *win) {
    const SDL_MessageBoxButtonData buttons[] = {
            {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes (Save)"},
            {0,                                       2, "No (Don't Save)"},
            {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"}
    };
    const SDL_MessageBoxColorScheme colorScheme = {{
        {255, 255, 255}, // background
        {0, 0, 0}, // text
        {100, 100, 100}, // button border
        {220, 220, 220}, // button background
        {0, 0, 0}  // button selected
    }};
    SDL_MessageBoxData data = {};
    data.flags = SDL_MESSAGEBOX_INFORMATION;
    data.window = win;
    data.title = "New Project";
    data.message = "Do you want to save the current project before creating a new one?";
    data.numbuttons = 3;
    data.buttons = buttons;
    data.colorScheme = &colorScheme;
    int buttonid = 0;
    SDL_ShowMessageBox(&data, &buttonid);
    return buttonid; // 1=Yes, 2=No, 0=Cancel
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);
    SDL_Window *win = SDL_CreateWindow(
            "main window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("calibrib.ttf", 14);
    TTF_Font *font2 = TTF_OpenFont("calibrib.ttf", 22);
    TTF_Font *font3 = TTF_OpenFont("calibrib.ttf", 17);

//بارگذاری قارچ اسپرایت
    marioTexture = IMG_LoadTexture(ren, "mario.png");
    SDL_SetTextureBlendMode(marioTexture, SDL_BLENDMODE_BLEND);

    vector<BlockType> blockTypes = {
            {{70,  150, 255, 255}, {0, 50,  80, 50}, "motion"},
            {{150, 100, 255, 255}, {0, 100, 80, 50}, "looks"},
            {{210, 100, 210, 255}, {0, 150, 80, 50}, "sound"},
            {{255, 210, 0,   255}, {0, 200, 80, 50}, "events"},
            {{255, 170, 25,  255}, {0, 250, 80, 50}, "control"},
            {{90,  180, 210, 255}, {0, 300, 80, 50}, "senses"},
            {{90,  190, 90,  255}, {0, 350, 80, 50}, "operators"},
            {{255, 140, 25,  255}, {0, 400, 80, 50}, "variables"}};

    int logo_w, logo_h;
    SDL_Texture *logo = IMG_LoadTexture(ren, "logo.png");
    SDL_QueryTexture(logo, NULL, NULL, &logo_w, &logo_h);
    SDL_Rect logoSize = {900, -35, 120, 120};

    SDL_Rect fileBtn = {10, 8, 60, 30};
    SDL_Rect fileItemNew = {10, MENU_H, 120, 28};
    SDL_Rect fileItemSave = {10, MENU_H + 28, 120, 28};
    SDL_Rect fileItemLoad = {10, MENU_H + 56, 120, 28};
    bool fileMenuOpen = false;

    string projectPath = getProjectPath();
    cout << "Project file path: " << projectPath << endl;

    bool penExtensionAdded = false;
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            //  مدیریت تایپ اعداد در فیلدها
            if (e.type == SDL_TEXTINPUT && activeField > 0) {
                if (activeField == 1) marioData.xStr += e.text.text;
                else if (activeField == 2) marioData.yStr += e.text.text;
                else if (activeField == 3) marioData.sizeStr += e.text.text;
                else if (activeField == 4) marioData.angleStr += e.text.text;
                updateMarioFromInputs();
            }
            if (e.type == SDL_KEYDOWN && activeField > 0) {
                if (e.key.keysym.sym == SDLK_BACKSPACE) {
                    string* s = (activeField==1)? &marioData.xStr : (activeField==2)? &marioData.yStr : (activeField==3)? &marioData.sizeStr : &marioData.angleStr;
                    if (!s->empty()) { s->pop_back(); updateMarioFromInputs(); }
                }
                if (e.key.keysym.sym == SDLK_RETURN) { activeField = 0; SDL_StopTextInput(); }
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;
                bool consumed = false;

                // چک کردن کلیک روی فیلدهای ورودی
                if (pointInRect(mx, my, xInputRect)) { activeField = 1; SDL_StartTextInput(); }
                else if (pointInRect(mx, my, yInputRect)) { activeField = 2; SDL_StartTextInput(); }
                else if (pointInRect(mx, my, sizeInputRect)) { activeField = 3; SDL_StartTextInput(); }
                else if (pointInRect(mx, my, angleInputRect)) { activeField = 4; SDL_StartTextInput(); }
                else { activeField = 0; SDL_StopTextInput(); }

                // بررسی باز کردن منوی اکستنشن
                if (extensionMenuOpen) {
                    if (pointInRect(mx, my, getBackBtn)) {
                        extensionMenuOpen = false;
                        cout << "Get Back clicked" << endl;
                        consumed = true;
                    } else if (pointInRect(mx, my, penBtn)) {
                        if (!penExtensionAdded) {
                            // اضافه کردن بلوک مداد
                            BlockType penBlock = {{70, 220, 115, 255}, {0, 450, 80, 50}, "pen"};
                            blockTypes.push_back(penBlock);
                            penExtensionAdded = true;
                            cout << "Pen extension added" << endl;
                        }
                        extensionMenuOpen = false;
                        consumed = true;
                    }
                    if (consumed) continue;
                }

                if (pointInRect(mx, my, addExtensionBtn)) {
                    extensionMenuOpen = true;
                    cout << "Add Extension clicked" << endl;
                    consumed = true;
                }

                if (pointInRect(mx, my, fileBtn)) {
                    fileMenuOpen = !fileMenuOpen;
                    cout << "File menu toggled: " << fileMenuOpen << endl;
                    consumed = true;
                } else if (fileMenuOpen) {
                    if (pointInRect(mx, my, fileItemNew)) {
                        // پرسش برای ذخیره قبل از پاک کردن
                        if (!instances.empty()) {
                            int res = confirmSaveBeforeNew(win);
                            if (res == 1) { // Yes
                                bool ok = saveBlocks(projectPath, instances);
                                SDL_ShowSimpleMessageBox(ok ? SDL_MESSAGEBOX_INFORMATION : SDL_MESSAGEBOX_ERROR,
                                                         "Save", ok ? "Saved successfully." : "Save failed!",
                                                         win);
                                if (ok) instances.clear();
                            } else if (res == 2) // No
                                instances.clear();
                        } else
                            // اگر چیزی نیست، مستقیم پاک کن
                            instances.clear();

                        fileMenuOpen = false;
                        cout << "New clicked" << endl;
                        consumed = true;
                    } else if (pointInRect(mx, my, fileItemSave)) {
                        bool ok = saveBlocks(projectPath, instances);
                        cout << "Save clicked: " << (ok ? "OK" : "FAIL") << endl;
                        SDL_ShowSimpleMessageBox(ok ? SDL_MESSAGEBOX_INFORMATION : SDL_MESSAGEBOX_ERROR,
                                                 "Save", ok ? "Saved successfully." : "Save failed!",
                                                 win);
                        fileMenuOpen = false;
                        consumed = true;
                    } else if (pointInRect(mx, my, fileItemLoad)) {
                        bool ok = loadBlocks(projectPath, instances);
                        cout << "Load clicked: " << (ok ? "OK" : "FAIL") << endl;
                        SDL_ShowSimpleMessageBox(ok ? SDL_MESSAGEBOX_INFORMATION : SDL_MESSAGEBOX_ERROR,
                                                 "Load", ok ? "Loaded successfully." : "Load failed!",
                                                 win);
                        fileMenuOpen = false;
                        consumed = true;
                    } else
                        fileMenuOpen = false;
                }
                if (my < MENU_H) consumed = true;
                if (consumed) continue;

                // بررسی کلیک بلوک ها
                for (const auto &i: blockTypes) {
                    if (pointInRect(mx, my, i.protoRect)) {
                        // نمایش شبه بلوک ها
                        createSubBlocks(i);
                        consumed = true;
                        break;
                    }
                }

                if (consumed) continue;

                // چک کلیک کردن روی بلوک های چپ
                bool subBlockClicked = false;
                for (const auto &i: currentSubBlocks) {
                    if (pointInRect(mx, my, i.rect)) {
                        // ساخت شبه بلوک جدید قابل درگ
                        BlockInstance b;
                        b.typeIndex = i.typeIndex;
                        b.rect = i.rect;
                        b.dragging = true;
                        b.dragStartX = mx - b.rect.x;
                        b.dragStartY = my - b.rect.y;
                        b.lastValidX = b.rect.x;
                        b.lastValidY = b.rect.y;
                        b.customName = i.name;
                        instances.push_back(b);
                        subBlockClicked = true;
                        break;
                    }
                }

                if (subBlockClicked) continue;

                // چک کلیک کردن روی شبه بلوک موجود در پنل کاری
                bool picked = false;
                for (int i = (int) instances.size() - 1; i >= 0; --i) {
                    if (pointInRect(mx, my, instances[i].rect)) {
                        instances[i].dragging = true;
                        instances[i].dragStartX = mx - instances[i].rect.x;
                        instances[i].dragStartY = my - instances[i].rect.y;
                        BlockInstance temp = instances[i];
                        instances.erase(instances.begin() + i);
                        instances.push_back(temp);
                        picked = true;
                        break;
                    }
                }
            }

            if (e.type == SDL_MOUSEMOTION) {
                int mx = e.motion.x, my = e.motion.y;
                if (!instances.empty()) {
                    BlockInstance &b = instances.back();
                    if (b.dragging) {
                        b.rect.x = mx - b.dragStartX;
                        b.rect.y = my - b.dragStartY;
                    }
                }
            }

            if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                if (!instances.empty()) {
                    BlockInstance &b = instances.back();
                    if (b.dragging) {
                        b.dragging = false;
                        int cx = b.rect.x + b.rect.w / 2, cy = b.rect.y + b.rect.h / 2;

                        if (pointInRect(cx, cy, workspace)) {
                            b.lastValidX = b.rect.x;
                            b.lastValidY = b.rect.y;
                        } else if (pointInRect(cx, cy, stagePanel)) {
                            b.rect.x = b.lastValidX;
                            b.rect.y = b.lastValidY;
                            if (!pointInRect(b.rect.x + b.rect.w / 2, b.rect.y + b.rect.h / 2, workspace))
                                instances.pop_back();
                        } else {
                            instances.pop_back();
                        }
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
        SDL_RenderClear(ren);

        // رسم بار بنفش بالایی
        boxRGBA(ren, 0, 0, WINDOW_WIDTH, MENU_H, 160, 70, 165, 200);
        SDL_RenderCopy(ren, logo, NULL, &logoSize);

        writeLeftText(ren, font2, "File", fileBtn, {255, 255, 255, 255}, 10);

        drawRect(ren, leftPanel, {255, 255, 255, 255});
        drawRect(ren, workspace, {248, 249, 255, 255});

        // رسم بصری دو بخش پنل سمت راست
        drawRect(ren, spriteArea, {240, 244, 255, 255}); // بخش بالایی (محل حرکت)
        SDL_Rect bottomPart = {710, 325, 290, 275};
        drawRect(ren, bottomPart, {210, 210, 220, 255});

        // --- NEW: رسم پنل پایینی و فیلدها ---

        // فیلد X و Y
        writeLeftText(ren, font, "X:", {720, 350, 30, 25}, {0,0,0,255}, 0);
        drawRect(ren, xInputRect, (activeField == 1) ? SDL_Color{255,255,200,255} : SDL_Color{255,255,255,255});
        writeLeftText(ren, font, marioData.xStr, xInputRect, {0,0,0,255}, 5);

        writeLeftText(ren, font, "Y:", {820, 350, 30, 25}, {0,0,0,255}, 0);
        drawRect(ren, yInputRect, (activeField == 2) ? SDL_Color{255,255,200,255} : SDL_Color{255,255,255,255});
        writeLeftText(ren, font, marioData.yStr, yInputRect, {0,0,0,255}, 5);

        // فیلد Size و Direction
        writeLeftText(ren, font, "Size:", {715, 390, 45, 25}, {0,0,0,255}, 0);
        drawRect(ren, sizeInputRect, (activeField == 3) ? SDL_Color{255,255,200,255} : SDL_Color{255,255,255,255});
        writeLeftText(ren, font, marioData.sizeStr, sizeInputRect, {0,0,0,255}, 5);

        writeLeftText(ren, font, "Dir:", {820, 390, 40, 25}, {0,0,0,255}, 0);
        drawRect(ren, angleInputRect, (activeField == 4) ? SDL_Color{255,255,200,255} : SDL_Color{255,255,255,255});
        writeLeftText(ren, font, marioData.angleStr, angleInputRect, {0,0,0,255}, 5);
        //خطوط جداکننده
        vlineRGBA(ren, 280, 50, WINDOW_HEIGHT, 0, 0, 0, 255);
        vlineRGBA(ren, 710, 50, WINDOW_HEIGHT, 0, 0, 0, 255);
        //خط افقی جدا کننده
        hlineRGBA(ren, 710, 1000, 325, 0, 0, 0, 255);

        // رسم بلوک ها
        for (auto &bt: blockTypes) {
            drawRect(ren, bt.protoRect, bt.color);
            writeCenteredText(ren, font3, bt.name, bt.protoRect);
        }

        // رسم شبه بلوک ها
        for (auto &sb: currentSubBlocks) {
            drawRect(ren, sb.rect, sb.color);
            string text = sb.name;
            writeCenteredText(ren, font, text, sb.rect);
        }

        // رسم شبه بلوک های قابل درگ
        for (auto &b: instances) {
            drawRect(ren, b.rect, blockTypes[b.typeIndex].color);
            string text = b.customName;
            writeCenteredText(ren, font, text, b.rect);
        }

        // رسم دکمه افزودن اکستنشن
        boxRGBA(ren, addExtensionBtn.x, addExtensionBtn.y,
                addExtensionBtn.x + addExtensionBtn.w, addExtensionBtn.y + addExtensionBtn.h,
                100, 150, 200, 255);
        writeCenteredText(ren, font, "Add Extension", addExtensionBtn);

        if (fileMenuOpen) {
            boxRGBA(ren, fileItemNew.x, fileItemNew.y, fileItemNew.x + fileItemNew.w, fileItemNew.y + fileItemNew.h, 160,
                    70, 165, 255);
            boxRGBA(ren, fileItemSave.x, fileItemSave.y, fileItemSave.x + fileItemSave.w,
                    fileItemSave.y + fileItemSave.h, 160, 70, 165, 255);
            boxRGBA(ren, fileItemLoad.x, fileItemLoad.y, fileItemLoad.x + fileItemLoad.w,
                    fileItemLoad.y + fileItemLoad.h, 160, 70, 165, 255);

            writeLeftText(ren, font, "New Project", fileItemNew, {255, 255, 255, 255}, 10);
            writeLeftText(ren, font, "Save Project", fileItemSave, {255, 255, 255, 255}, 10);
            writeLeftText(ren, font, "Load Project", fileItemLoad, {255, 255, 255, 255}, 10);
        }
        if (marioTexture) {
            SDL_Point center = { marioRect.w / 2, marioRect.h / 2 };
            double finalAngle = (double)marioData.angle - 90.0;
            SDL_RenderCopyEx(ren, marioTexture, NULL, &marioRect, finalAngle, &center, SDL_FLIP_NONE);
        }

        // رسم منوی اکستنشن اگر باز است
        if (extensionMenuOpen) {
            // پس زمینه نیمه شفاف
            boxRGBA(ren, 0, 0,WINDOW_WIDTH, WINDOW_HEIGHT,255, 255, 255, 255);

            // دکمه بازگشت
            boxRGBA(ren, getBackBtn.x, getBackBtn.y,
                    getBackBtn.x + getBackBtn.w, getBackBtn.y + getBackBtn.h,
                    200, 100, 100, 255);
            writeCenteredText(ren, font2, "<- Back", getBackBtn);

            // دکمه قلم
            boxRGBA(ren, penBtn.x, penBtn.y,
                    penBtn.x + penBtn.w, penBtn.y + penBtn.h,
                    70, 220, 115, 255);
            writeCenteredText(ren, font2, "Pen", penBtn);

            // اگر قلم قبلا اضافه شده، پیام نمایش بده
            if (penExtensionAdded) {
                SDL_Rect msgRect = {50, 250, 300, 40};
                TTF_Font *fontError = TTF_OpenFont("calibrib.ttf", 26);
                writeLeftText(ren, fontError, "Pen already added!", msgRect, {0, 0, 0, 255}, 0);
            }
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_DestroyTexture(logo);
    SDL_DestroyTexture(marioTexture);
    IMG_Quit();
    SDL_Quit();
    return 0;
}