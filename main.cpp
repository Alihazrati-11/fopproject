#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfx.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cctype>
#include <cstdlib>
#include <sstream>

using namespace std;

const int WINDOW_WIDTH = 1000, WINDOW_HEIGHT = 600;
const int MENU_H = 48;

// مختصات اولیه ماریو
const int MARIO_ORIGIN_X = 810, MARIO_ORIGIN_Y = 100;

struct SpriteData {
    string name = "Mario";
    string xStr = "0", yStr = "0", sizeStr = "100", angleStr = "90";
    int x = 0, y = 0, size = 100, angle = 90;
} marioData;

// متغیرهای looks
bool marioVisible = true, isThinking = false, messagePersistent = false;
string currentMessage;
Uint32 messageTimeout = 0;
int costumeIndex = 0, backdropIndex = 0;

// مشخص کردن اینکه کدام فیلد در حال ویرایش است (0: هیچکدام، 1: X، 2: Y، 3: Size، 4: Angle)
int activeField = 0;

// پوینتر برای نوشتن درون شبه بلوک ها
string* activeParamStr = nullptr;

// فیلدهای مستطیلی برای کلیک کاربر
SDL_Rect xInputRect = {760, 350, 60, 25};
SDL_Rect yInputRect = {860, 350, 60, 25};
SDL_Rect sizeInputRect = {760, 390, 60, 25};
SDL_Rect angleInputRect = {860, 390, 60, 25};

// ناحیه ها
SDL_Rect leftPanel = {80, 50, 200, 550};
SDL_Rect workspace = {280, 50, 470, 550};
SDL_Rect stagePanel = {710, 50, 290, 550};

// محل ماریو در مربع بالایی
SDL_Rect spriteArea = {710, 50, 290, 275};
SDL_Texture *marioTexture;
SDL_Rect marioRect = {MARIO_ORIGIN_X, MARIO_ORIGIN_Y, 90, 90};

// محلی که شبه بلوک ها ظاهر می شوند
SDL_Rect subBlockArea = {90, 60, 180, 200};

// منوی اکستنشن و دکمه آن
SDL_Rect addExtensionBtn = {0, WINDOW_HEIGHT - 40, 120, 40};
bool extensionMenuOpen = false;
SDL_Rect getBackBtn = {10, 40, 120, 50};
SDL_Rect penBtn = {300, 50, 200, 200};

void updateMarioFromInputs() {
    bool updated = false;

    if (!marioData.xStr.empty()) {
        char *endptr;
        long val = strtol(marioData.xStr.c_str(), &endptr, 10);
        if (*endptr == '\0' && endptr != marioData.xStr.c_str()) {
            marioData.x = val;
            updated = true;
        }
    }
    if (!marioData.yStr.empty()) {
        char *endptr;
        long val = strtol(marioData.yStr.c_str(), &endptr, 10);
        if (*endptr == '\0' && endptr != marioData.yStr.c_str()) {
            marioData.y = val;
            updated = true;
        }
    }
    if (!marioData.sizeStr.empty()) {
        char *endptr;
        long val = strtol(marioData.sizeStr.c_str(), &endptr, 10);
        if (*endptr == '\0' && endptr != marioData.sizeStr.c_str()) {
            if (val > 0 && val <= 500) {
                marioData.size = val;
                updated = true;
            }
        }
    }
    if (!marioData.angleStr.empty()) {
        char *endptr;
        long val = strtol(marioData.angleStr.c_str(), &endptr, 10);
        if (*endptr == '\0' && endptr != marioData.angleStr.c_str()) {
            int angle = val % 360;
            if (angle < 0) angle += 360;
            marioData.angle = angle;
            updated = true;
        }
    }
    if (updated) {
        marioRect.x = MARIO_ORIGIN_X + marioData.x;
        marioRect.y = MARIO_ORIGIN_Y + marioData.y;
        double scale = marioData.size / 100.0;
        marioRect.w = 90 * scale;
        marioRect.h = 90 * scale;
    }
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
    int typeIndex, paramCount = 0;
    string defaultParam1, defaultParam2;
};

struct BlockInstance {
    int typeIndex;
    SDL_Rect rect;
    int lastValidX, lastValidY;
    bool dragging = false;
    int dragStartX = 0, dragStartY = 0;
    string customName;
    int paramCount = 0;
    string param1, param2;
    SDL_Rect param1Rect = {0, 0, 0, 0};
    SDL_Rect param2Rect = {0, 0, 0, 0};
};

vector<BlockInstance> instances;
vector<SubBlock> currentSubBlocks;

// منوهای بازشونده شبه بلوک ها
bool dropdownOpen = false;
BlockInstance* activeDropdownBlock = nullptr;

string operatorResultText = "";
bool operatorResultError = false;

vector<string> getDropdownOptions(const string& cmd) {
    if (cmd == "switch backdrop") return {"Bg 1", "Bg 2", "Bg 3"};
    if (cmd == "switch costume") return {"Cos 1", "Cos 2"};
    return {};
}

bool readNumberValue(const string &text, double &outValue) {
    if (text.empty()) return false;
    const char *ptr = text.c_str();
    char *endptr = nullptr;
    outValue = strtod(ptr, &endptr);
    if (endptr == ptr) return false;
    while (*endptr != '\0' && isspace(static_cast<unsigned char>(*endptr))) endptr++;
    if (*endptr != '\0') return false;
    return true;
}

string formatNumber(double value) {
    ostringstream ss;
    ss.setf(ios::fixed);
    ss.precision(2);
    ss << value;
    string out = ss.str();
    while (out.size() > 1 && out.back() == '0') out.pop_back();
    if (!out.empty() && out.back() == '.') out.pop_back();
    return out;
}

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
        string names[12] = {
                "say_for", "say", "think_for", "think",
                "switch costume", "next costume", "switch backdrop", "next backdrop",
                "change size", "set size", "show", "hide"
        };
        int paramCounts[12] = {2, 1, 2, 1, 1, 0, 1, 0, 1, 1, 0, 0};
        string defP1[12] = {"Hello!", "Hello!", "Hmm...", "Hmm...", "Cos 1", "", "Bg 1", "", "10", "100", "", ""};
        string defP2[12] = {"2", "", "2", "", "", "", "", "", "", "", "", ""};

        for (int i = 0; i < 12; i++) {
            SubBlock sb;
            sb.color = blockType.color;
            sb.name = names[i];
            sb.typeIndex = 1;
            sb.paramCount = paramCounts[i];
            sb.defaultParam1 = defP1[i];
            sb.defaultParam2 = defP2[i];
            sb.rect = {startX, startY + (i * 38), 170, 30};
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
        string names[5] = {"add", "subtract", "multiply", "divide", "pick random"};
        string defP1[5] = {"0", "0", "1", "1", "1"};
        string defP2[5] = {"0", "0", "1", "1", "10"};
        for (int i = 0; i < 5; i++) {
            SubBlock sb;
            sb.color = blockType.color;
            sb.rect = {startX, startY + (i * 38), 180, 30};
            sb.name = names[i];
            sb.typeIndex = 6;
            sb.paramCount = 2;
            sb.defaultParam1 = defP1[i];
            sb.defaultParam2 = defP2[i];
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
            sb.typeIndex = 8;
            currentSubBlocks.push_back(sb);
        }
    }
}

void executeLooksCommand(const string& cmd, string p1, const string& p2, Uint32 currentTime) {
    if (cmd == "show")
        marioVisible = true;
    else if (cmd == "hide")
        marioVisible = false;
    else if (cmd == "change size") {
        int change = 0;
        if (!p1.empty()) {
            bool valid = true;
            for (size_t i = 0; i < p1.length(); i++) {
                if (i == 0 && p1[i] == '-') continue;
                if (!isdigit(p1[i])) {
                    valid = false;
                    break;
                }
            }
            if (valid)
                change = stoi(p1);
        }
        int sz = marioData.size;
        sz += change;
        if (sz < 1) sz = 1;
        if (sz > 500) sz = 500;
        marioData.sizeStr = to_string(sz);
        updateMarioFromInputs();
    } else if (cmd == "set size") {
        marioData.sizeStr = p1;
        updateMarioFromInputs();
    } else if (cmd == "say_for") {
        currentMessage = p1;
        isThinking = false;
        messagePersistent = false;
        int duration = 2;
        if (!p2.empty()) {
            bool valid = true;
            for (char c : p2) {
                if (!isdigit(c)) {
                    valid = false;
                    break;
                }
            }
            if (valid)
                duration = stoi(p2);
        }
        messageTimeout = currentTime + (duration * 1000);
    } else if (cmd == "say") {
        currentMessage = p1;
        isThinking = false;
        messagePersistent = true;
    } else if (cmd == "think_for") {
        currentMessage = p1;
        isThinking = true;
        messagePersistent = false;
        int duration = 2;
        if (!p2.empty()) {
            bool valid = true;
            for (char c : p2) {
                if (!isdigit(c)) {
                    valid = false;
                    break;
                }
            }
            if (valid)
                duration = stoi(p2);
        }
        messageTimeout = currentTime + (duration * 1000);
    } else if (cmd == "think") {
        currentMessage = p1;
        isThinking = true;
        messagePersistent = true;
    } else if (cmd == "switch costume") {
        if (p1 == "Cos 1") costumeIndex = 0;
        else if (p1 == "Cos 2") costumeIndex = 1;
    } else if (cmd == "next costume")
        costumeIndex = (costumeIndex + 1) % 2;
    else if (cmd == "switch backdrop") {
        if (p1 == "Bg 1") backdropIndex = 0;
        else if (p1 == "Bg 2") backdropIndex = 1;
        else if (p1 == "Bg 3") backdropIndex = 2;
    } else if (cmd == "next backdrop")
        backdropIndex = (backdropIndex + 1) % 3;
}

void executeOperatorCommand(const BlockInstance &b) {
    if (b.customName == "pick random") {
        double first = 0.0, second = 0.0;
        bool ok1 = readNumberValue(b.param1, first);
        bool ok2 = readNumberValue(b.param2, second);
        if (!ok1 || !ok2) {
            operatorResultError = true;
            operatorResultText = "Invalid input";
            return;
        }
        int minVal = static_cast<int>(first);
        int maxVal = static_cast<int>(second);
        if (minVal > maxVal) {
            int tmp = minVal;
            minVal = maxVal;
            maxVal = tmp;
        }
        int range = maxVal - minVal + 1;
        if (range <= 0) {
            operatorResultError = true;
            operatorResultText = "Invalid range";
            return;
        }
        int value = minVal + (rand() % range);
        operatorResultError = false;
        operatorResultText = "random: " + to_string(value);
        return;
    }

    double a = 0.0, c = 0.0;
    bool okA = readNumberValue(b.param1, a);
    bool okC = readNumberValue(b.param2, c);
    if (!okA || !okC) {
        operatorResultError = true;
        operatorResultText = "Invalid input";
        return;
    }

    double result = 0.0;
    string opSymbol;
    if (b.customName == "add") {
        result = a + c;
        opSymbol = " + ";
    } else if (b.customName == "subtract") {
        result = a - c;
        opSymbol = " - ";
    } else if (b.customName == "multiply") {
        result = a * c;
        opSymbol = " * ";
    } else if (b.customName == "divide") {
        if (c == 0.0) {
            operatorResultError = true;
            operatorResultText = "Cannot divide by zero";
            return;
        }
        result = a / c;
        opSymbol = " / ";
    } else {
        operatorResultError = true;
        operatorResultText = "Unsupported";
        return;
    }

    operatorResultError = false;
    operatorResultText = formatNumber(a) + opSymbol + formatNumber(c) + " = " + formatNumber(result);
}

void writeCenteredText(SDL_Renderer *ren, TTF_Font *font, const string &text, SDL_Rect rect, SDL_Color color = {255, 255, 255, 255}) {
    if (text.empty()) return;
    SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    int tw, th;
    SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
    SDL_Rect dst = {rect.x + (rect.w - tw) / 2, rect.y + (rect.h - th) / 2, tw, th};
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

void writeLeftText(SDL_Renderer *ren, TTF_Font *font, const string &text, SDL_Rect rect, SDL_Color color, int pad) {
    if (text.empty()) return;
    SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surf) return;
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
            << b.paramCount << "\n"
            << b.customName << "\n";

        if (b.paramCount >= 1) out << b.param1 << "\n";
        if (b.paramCount >= 2) out << b.param2 << "\n";
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
        in >> b.typeIndex >> b.rect.x >> b.rect.y >> b.rect.w >> b.rect.h >> b.paramCount;
        in.ignore();
        getline(in, b.customName);
        if (b.paramCount >= 1) getline(in, b.param1);
        if (b.paramCount >= 2) getline(in, b.param2);

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
            {0, 2, "No (Don't Save)"},
            {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"}
    };
    const SDL_MessageBoxColorScheme colorScheme = {{
                                                           {255, 255, 255}, {0, 0, 0},
                                                           {100, 100, 100}, {220, 220, 220}, {0, 0, 0}
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
    return buttonid;
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);
    SDL_Window *win = SDL_CreateWindow(
            "Merged Scratch-like Program",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("calibrib.ttf", 14);
    TTF_Font *font2 = TTF_OpenFont("calibrib.ttf", 22);
    TTF_Font *font3 = TTF_OpenFont("calibrib.ttf", 17);

    marioTexture = IMG_LoadTexture(ren, "mario.png");
    SDL_SetTextureBlendMode(marioTexture, SDL_BLENDMODE_BLEND);

    srand((unsigned int)SDL_GetTicks());

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
    if (logo) SDL_QueryTexture(logo, NULL, NULL, &logo_w, &logo_h);
    SDL_Rect logoSize = {900, -35, 120, 120};

    SDL_Rect fileBtn = {10, 8, 60, 30};
    SDL_Rect fileItemNew = {10, MENU_H, 120, 28};
    SDL_Rect fileItemSave = {10, MENU_H + 28, 120, 28};
    SDL_Rect fileItemLoad = {10, MENU_H + 56, 120, 28};
    bool fileMenuOpen = false;

    string projectPath = getProjectPath();
    bool penExtensionAdded = false, running = true;

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        SDL_Event e;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            if (e.type == SDL_TEXTINPUT) {
                if (activeField > 0) {
                    string *targetStr = nullptr;
                    if (activeField == 1) targetStr = &marioData.xStr;
                    else if (activeField == 2) targetStr = &marioData.yStr;
                    else if (activeField == 3) targetStr = &marioData.sizeStr;
                    else if (activeField == 4) targetStr = &marioData.angleStr;

                    if (targetStr) {
                        const char *input = e.text.text;
                        for (int i = 0; input[i] != '\0'; ++i) {
                            char c = input[i];
                            bool allowed = false;
                            if (activeField == 3) {
                                if (isdigit(c)) allowed = true;
                            } else {
                                if (isdigit(c)) allowed = true;
                                else if (c == '-' && targetStr->empty()) allowed = true;
                            }
                            if (allowed) *targetStr += c;
                        }
                        updateMarioFromInputs();
                    }
                } else if (activeParamStr != nullptr)
                    *activeParamStr += e.text.text;
            }

            if (e.type == SDL_KEYDOWN) {
                if (activeField > 0) {
                    string *s = (activeField == 1) ? &marioData.xStr :
                                (activeField == 2) ? &marioData.yStr :
                                (activeField == 3) ? &marioData.sizeStr :
                                &marioData.angleStr;
                    if (e.key.keysym.sym == SDLK_BACKSPACE && !s->empty()) {
                        s->pop_back();
                        updateMarioFromInputs();
                    }
                    if (e.key.keysym.sym == SDLK_RETURN) {
                        activeField = 0;
                        SDL_StopTextInput();
                    }
                }
                else if (activeParamStr != nullptr) {
                    if (e.key.keysym.sym == SDLK_BACKSPACE && !activeParamStr->empty())
                        activeParamStr->pop_back();
                    if (e.key.keysym.sym == SDLK_RETURN) {
                        activeParamStr = nullptr;
                        SDL_StopTextInput();
                    }
                }
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;
                bool consumed = false;

                if (dropdownOpen && activeDropdownBlock != nullptr) {
                    vector<string> opts = getDropdownOptions(activeDropdownBlock->customName);
                    for (size_t j = 0; j < opts.size(); j++) {
                        SDL_Rect optRect = {
                                activeDropdownBlock->param1Rect.x,
                                activeDropdownBlock->param1Rect.y + activeDropdownBlock->param1Rect.h + (int)(j * 20),
                                80, 20
                        };
                        if (pointInRect(mx, my, optRect)) {
                            activeDropdownBlock->param1 = opts[j];
                            break;
                        }
                    }
                    dropdownOpen = false;
                    activeDropdownBlock = nullptr;
                    consumed = true;
                }
                if (consumed) continue;

                bool clickedInput = false;
                if (pointInRect(mx, my, xInputRect)) {
                    activeField = 1;
                    clickedInput = true;
                } else if (pointInRect(mx, my, yInputRect)) {
                    activeField = 2;
                    clickedInput = true;
                } else if (pointInRect(mx, my, sizeInputRect)) {
                    activeField = 3;
                    clickedInput = true;
                } else if (pointInRect(mx, my, angleInputRect)) {
                    activeField = 4;
                    clickedInput = true;
                } else
                    activeField = 0;

                if (extensionMenuOpen) {
                    if (pointInRect(mx, my, getBackBtn)) {
                        extensionMenuOpen = false;
                        consumed = true;
                    } else if (pointInRect(mx, my, penBtn)) {
                        if (!penExtensionAdded) {
                            BlockType penBlock = {{70, 220, 115, 255}, {0, 450, 80, 50}, "pen"};
                            blockTypes.push_back(penBlock);
                            penExtensionAdded = true;
                        }
                        extensionMenuOpen = false;
                        consumed = true;
                    }
                    if (consumed) continue;
                }

                if (pointInRect(mx, my, addExtensionBtn)) {
                    extensionMenuOpen = true;
                    consumed = true;
                }

                if (pointInRect(mx, my, fileBtn)) {
                    fileMenuOpen = !fileMenuOpen;
                    consumed = true;
                } else if (fileMenuOpen) {
                    if (pointInRect(mx, my, fileItemNew)) {
                        if (!instances.empty()) {
                            int res = confirmSaveBeforeNew(win);
                            if (res == 1) {
                                bool ok = saveBlocks(projectPath, instances);
                                SDL_ShowSimpleMessageBox(ok ? SDL_MESSAGEBOX_INFORMATION : SDL_MESSAGEBOX_ERROR,
                                                         "Save", ok ? "Saved successfully." : "Save failed!", win);
                                if (ok) instances.clear();
                            } else if (res == 2) instances.clear();
                        } else instances.clear();
                        fileMenuOpen = false;
                        consumed = true;
                    } else if (pointInRect(mx, my, fileItemSave)) {
                        bool ok = saveBlocks(projectPath, instances);
                        SDL_ShowSimpleMessageBox(ok ? SDL_MESSAGEBOX_INFORMATION : SDL_MESSAGEBOX_ERROR,
                                                 "Save", ok ? "Saved successfully." : "Save failed!", win);
                        fileMenuOpen = false;
                        consumed = true;
                    } else if (pointInRect(mx, my, fileItemLoad)) {
                        bool ok = loadBlocks(projectPath, instances);
                        SDL_ShowSimpleMessageBox(ok ? SDL_MESSAGEBOX_INFORMATION : SDL_MESSAGEBOX_ERROR,
                                                 "Load", ok ? "Loaded successfully." : "Load failed!", win);
                        fileMenuOpen = false;
                        consumed = true;
                    } else fileMenuOpen = false;
                }
                if (my < MENU_H) consumed = true;
                if (consumed) continue;

                for (const auto &i: blockTypes) {
                    if (pointInRect(mx, my, i.protoRect)) {
                        createSubBlocks(i);
                        consumed = true;
                        break;
                    }
                }
                if (consumed) continue;

                bool subBlockClicked = false;
                for (const auto &i: currentSubBlocks) {
                    if (pointInRect(mx, my, i.rect)) {
                        BlockInstance b;
                        b.typeIndex = i.typeIndex;
                        b.rect = i.rect;
                        b.dragging = true;
                        b.dragStartX = mx - b.rect.x;
                        b.dragStartY = my - b.rect.y;
                        b.lastValidX = b.rect.x;
                        b.lastValidY = b.rect.y;
                        b.customName = i.name;
                        b.paramCount = i.paramCount;
                        b.param1 = i.defaultParam1;
                        b.param2 = i.defaultParam2;
                        instances.push_back(b);
                        subBlockClicked = true;
                        break;
                    }
                }
                if (subBlockClicked) continue;

                bool picked = false;
                for (int i = (int) instances.size() - 1; i >= 0; --i) {
                    if (instances[i].paramCount >= 1 && pointInRect(mx, my, instances[i].param1Rect)) {
                        if (instances[i].customName == "switch backdrop" || instances[i].customName == "switch costume") {
                            dropdownOpen = true;
                            activeDropdownBlock = &instances[i];
                            activeField = 0;
                            activeParamStr = nullptr;
                            SDL_StopTextInput();
                        } else {
                            activeParamStr = &instances[i].param1;
                            activeField = 0;
                            clickedInput = true;
                        }
                        picked = true;
                        break;
                    }
                    if (instances[i].paramCount >= 2 && pointInRect(mx, my, instances[i].param2Rect)) {
                        activeParamStr = &instances[i].param2;
                        activeField = 0;
                        clickedInput = true;
                        picked = true;
                        break;
                    }
                    if (pointInRect(mx, my, instances[i].rect)) {
                        instances[i].dragging = true;
                        instances[i].dragStartX = mx - instances[i].rect.x;
                        instances[i].dragStartY = my - instances[i].rect.y;

                        if (instances[i].typeIndex == 1)
                            executeLooksCommand(instances[i].customName, instances[i].param1, instances[i].param2, currentTime);
                        else if (instances[i].typeIndex == 6)
                            executeOperatorCommand(instances[i]);

                        BlockInstance temp = instances[i];
                        instances.erase(instances.begin() + i);
                        instances.push_back(temp);
                        picked = true;
                        break;
                    }
                }

                if (clickedInput)
                    SDL_StartTextInput();
                else {
                    activeParamStr = nullptr;
                    SDL_StopTextInput();
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
                        } else
                            instances.pop_back();
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
        SDL_RenderClear(ren);

        boxRGBA(ren, 0, 0, WINDOW_WIDTH, MENU_H, 160, 70, 165, 200);
        SDL_RenderCopy(ren, logo, NULL, &logoSize);
        writeLeftText(ren, font2, "File", fileBtn, {255, 255, 255, 255}, 10);

        drawRect(ren, leftPanel, {255, 255, 255, 255});
        drawRect(ren, workspace, {248, 249, 255, 255});

        SDL_Color bgColors[3] = {
                {240, 244, 255, 255},{200, 255, 200, 255},{255, 200, 200, 255}
        };
        drawRect(ren, spriteArea, bgColors[backdropIndex]);

        SDL_Rect bottomPart = {710, 325, 290, 275};
        drawRect(ren, bottomPart, {210, 210, 220, 255});

        writeLeftText(ren, font, "X :", {730, 350, 30, 25}, {0, 0, 0, 255}, 0);
        drawRect(ren, xInputRect, (activeField == 1) ? SDL_Color{255, 255, 200, 255} : SDL_Color{255, 255, 255, 255});
        writeLeftText(ren, font, marioData.xStr, xInputRect, {0, 0, 0, 255}, 5);

        writeLeftText(ren, font, "Y :", {830, 350, 30, 25}, {0, 0, 0, 255}, 0);
        drawRect(ren, yInputRect, (activeField == 2) ? SDL_Color{255, 255, 200, 255} : SDL_Color{255, 255, 255, 255});
        writeLeftText(ren, font, marioData.yStr, yInputRect, {0, 0, 0, 255}, 5);

        writeLeftText(ren, font, "Size :", {720, 390, 45, 25}, {0, 0, 0, 255}, 0);
        drawRect(ren, sizeInputRect, (activeField == 3) ? SDL_Color{255, 255, 200, 255} : SDL_Color{255, 255, 255, 255});
        writeLeftText(ren, font, marioData.sizeStr, sizeInputRect, {0, 0, 0, 255}, 5);

        writeLeftText(ren, font, "Dir :", {830, 390, 40, 25}, {0, 0, 0, 255}, 0);
        drawRect(ren, angleInputRect, (activeField == 4) ? SDL_Color{255, 255, 200, 255} : SDL_Color{255, 255, 255, 255});
        writeLeftText(ren, font, marioData.angleStr, angleInputRect, {0, 0, 0, 255}, 5);

        vlineRGBA(ren, 280, 50, WINDOW_HEIGHT, 0, 0, 0, 255);
        vlineRGBA(ren, 710, 50, WINDOW_HEIGHT, 0, 0, 0, 255);
        hlineRGBA(ren, 710, 1000, 325, 0, 0, 0, 255);

        for (auto &bt: blockTypes) {
            drawRect(ren, bt.protoRect, bt.color);
            writeCenteredText(ren, font3, bt.name, bt.protoRect);
        }

        for (auto &sb: currentSubBlocks) {
            drawRect(ren, sb.rect, sb.color);
            if (sb.paramCount == 1) {
                int tw = 0, th = 0;
                TTF_SizeText(font, sb.name.c_str(), &tw, &th);
                int p1w = (sb.name == "switch backdrop" || sb.name == "switch costume") ? 55 : 45;
                SDL_Rect p1Rect = {sb.rect.x + tw + 10, sb.rect.y + 4, p1w, sb.rect.h - 8};
                writeLeftText(ren, font, sb.name, sb.rect, {255, 255, 255, 255}, 5);
                drawRect(ren, p1Rect, {255, 255, 255, 255});
                writeLeftText(ren, font, sb.defaultParam1, p1Rect, {0, 0, 0, 255}, 2);
            } else if (sb.paramCount == 2) {
                if (sb.typeIndex == 1 && (sb.name == "say_for" || sb.name == "think_for")) {
                    string part1 = (sb.name == "say_for") ? "say" : "think";
                    int tw1 = 0, th = 0;
                    TTF_SizeText(font, part1.c_str(), &tw1, &th);
                    writeLeftText(ren, font, part1, sb.rect, {255, 255, 255, 255}, 5);

                    SDL_Rect p1Rect = {sb.rect.x + 5 + tw1 + 5, sb.rect.y + 4, 45, sb.rect.h - 8};
                    drawRect(ren, p1Rect, {255, 255, 255, 255});
                    writeLeftText(ren, font, sb.defaultParam1, p1Rect, {0, 0, 0, 255}, 2);

                    string part2 = " for ";
                    int tw2 = 0;
                    TTF_SizeText(font, part2.c_str(), &tw2, &th);
                    SDL_Rect part2Rect = {p1Rect.x + p1Rect.w, sb.rect.y, tw2, sb.rect.h};
                    writeLeftText(ren, font, part2, part2Rect, {255, 255, 255, 255}, 0);

                    SDL_Rect p2Rect = {part2Rect.x + tw2, sb.rect.y + 4, 30, sb.rect.h - 8};
                    drawRect(ren, p2Rect, {255, 255, 255, 255});
                    writeLeftText(ren, font, sb.defaultParam2, p2Rect, {0, 0, 0, 255}, 2);

                    string part3 = " s";
                    SDL_Rect part3Rect = {p2Rect.x + p2Rect.w, sb.rect.y, 20, sb.rect.h};
                    writeLeftText(ren, font, part3, part3Rect, {255, 255, 255, 255}, 0);
                } else if (sb.typeIndex == 6) {
                    if (sb.name == "pick random") {
                        string label = "pick random";
                        int twLabel = 0, th = 0;
                        TTF_SizeText(font, label.c_str(), &twLabel, &th);
                        SDL_Rect labelRect = {sb.rect.x + 5, sb.rect.y, twLabel, sb.rect.h};
                        writeLeftText(ren, font, label, labelRect, {255, 255, 255, 255}, 0);

                        SDL_Rect p1Rect = {labelRect.x + twLabel + 8, sb.rect.y + 4, 36, sb.rect.h - 8};
                        drawRect(ren, p1Rect, {255, 255, 255, 255});
                        writeLeftText(ren, font, sb.defaultParam1, p1Rect, {0, 0, 0, 255}, 2);

                        string mid = "to";
                        int twMid = 0;
                        TTF_SizeText(font, mid.c_str(), &twMid, &th);
                        SDL_Rect midRect = {p1Rect.x + p1Rect.w + 6, sb.rect.y, twMid, sb.rect.h};
                        writeLeftText(ren, font, mid, midRect, {255, 255, 255, 255}, 0);

                        SDL_Rect p2Rect = {midRect.x + twMid + 6, sb.rect.y + 4, 36, sb.rect.h - 8};
                        drawRect(ren, p2Rect, {255, 255, 255, 255});
                        writeLeftText(ren, font, sb.defaultParam2, p2Rect, {0, 0, 0, 255}, 2);
                    } else {
                        SDL_Rect p1Rect = {sb.rect.x + 6, sb.rect.y + 4, 40, sb.rect.h - 8};
                        drawRect(ren, p1Rect, {255, 255, 255, 255});
                        writeLeftText(ren, font, sb.defaultParam1, p1Rect, {0, 0, 0, 255}, 2);

                        string sign;
                        if (sb.name == "add") sign = "+";
                        else if (sb.name == "subtract") sign = "-";
                        else if (sb.name == "multiply") sign = "*";
                        else sign = "/";

                        SDL_Rect signRect = {p1Rect.x + p1Rect.w + 6, sb.rect.y, 18, sb.rect.h};
                        writeCenteredText(ren, font, sign, signRect);

                        SDL_Rect p2Rect = {signRect.x + signRect.w + 6, sb.rect.y + 4, 40, sb.rect.h - 8};
                        drawRect(ren, p2Rect, {255, 255, 255, 255});
                        writeLeftText(ren, font, sb.defaultParam2, p2Rect, {0, 0, 0, 255}, 2);
                    }
                } else
                    writeCenteredText(ren, font, sb.name, sb.rect);
            } else
                writeCenteredText(ren, font, sb.name, sb.rect);
        }

        for (auto &b: instances) {
            drawRect(ren, b.rect, blockTypes[b.typeIndex].color);
            b.param1Rect = {0, 0, 0, 0};
            b.param2Rect = {0, 0, 0, 0};

            if (b.paramCount == 1) {
                int tw = 0, th = 0;
                TTF_SizeText(font, b.customName.c_str(), &tw, &th);
                int p1w = (b.customName == "switch backdrop" || b.customName == "switch costume") ? 55 : 45;
                b.param1Rect = {b.rect.x + tw + 10, b.rect.y + 4, p1w, b.rect.h - 8};

                writeLeftText(ren, font, b.customName, b.rect, {255, 255, 255, 255}, 5);
                SDL_Color boxColor = (activeParamStr == &b.param1) ? SDL_Color{255, 200, 200, 255} : SDL_Color{255, 255, 255, 255};
                drawRect(ren, b.param1Rect, boxColor);
                writeLeftText(ren, font, b.param1, b.param1Rect, {0, 0, 0, 255}, 2);
            } else if (b.paramCount == 2) {
                if (b.customName == "say_for" || b.customName == "think_for") {
                    string part1 = (b.customName == "say_for") ? "say" : "think";
                    int tw1 = 0, th = 0;
                    TTF_SizeText(font, part1.c_str(), &tw1, &th);
                    writeLeftText(ren, font, part1, b.rect, {255, 255, 255, 255}, 5);

                    b.param1Rect = {b.rect.x + 5 + tw1 + 5, b.rect.y + 4, 45, b.rect.h - 8};
                    SDL_Color boxColor1 = (activeParamStr == &b.param1) ? SDL_Color{255, 200, 200, 255} : SDL_Color{255, 255, 255, 255};
                    drawRect(ren, b.param1Rect, boxColor1);
                    writeLeftText(ren, font, b.param1, b.param1Rect, {0, 0, 0, 255}, 2);

                    string part2 = " for ";
                    int tw2 = 0;
                    TTF_SizeText(font, part2.c_str(), &tw2, &th);
                    SDL_Rect part2Rect = {b.param1Rect.x + b.param1Rect.w, b.rect.y, tw2, b.rect.h};
                    writeLeftText(ren, font, part2, part2Rect, {255, 255, 255, 255}, 0);

                    b.param2Rect = {part2Rect.x + tw2, b.rect.y + 4, 30, b.rect.h - 8};
                    SDL_Color boxColor2 = (activeParamStr == &b.param2) ? SDL_Color{255, 200, 200, 255} : SDL_Color{255, 255, 255, 255};
                    drawRect(ren, b.param2Rect, boxColor2);
                    writeLeftText(ren, font, b.param2, b.param2Rect, {0, 0, 0, 255}, 2);

                    string part3 = " s";
                    SDL_Rect part3Rect = {b.param2Rect.x + b.param2Rect.w, b.rect.y, 20, b.rect.h};
                    writeLeftText(ren, font, part3, part3Rect, {255, 255, 255, 255}, 0);
                } else if (b.typeIndex == 6) {
                    if (b.customName == "pick random") {
                        string label = "pick random";
                        int twLabel = 0, th = 0;
                        TTF_SizeText(font, label.c_str(), &twLabel, &th);
                        SDL_Rect labelRect = {b.rect.x + 5, b.rect.y, twLabel, b.rect.h};
                        writeLeftText(ren, font, label, labelRect, {255, 255, 255, 255}, 0);

                        b.param1Rect = {labelRect.x + twLabel + 8, b.rect.y + 4, 40, b.rect.h - 8};
                        SDL_Color boxColor1 = (activeParamStr == &b.param1) ? SDL_Color{255, 200, 200, 255} : SDL_Color{255, 255, 255, 255};
                        drawRect(ren, b.param1Rect, boxColor1);
                        writeLeftText(ren, font, b.param1, b.param1Rect, {0, 0, 0, 255}, 2);

                        string mid = "to";
                        int twMid = 0;
                        TTF_SizeText(font, mid.c_str(), &twMid, &th);
                        SDL_Rect midRect = {b.param1Rect.x + b.param1Rect.w + 6, b.rect.y, twMid, b.rect.h};
                        writeLeftText(ren, font, mid, midRect, {255, 255, 255, 255}, 0);

                        b.param2Rect = {midRect.x + twMid + 6, b.rect.y + 4, 40, b.rect.h - 8};
                        SDL_Color boxColor2 = (activeParamStr == &b.param2) ? SDL_Color{255, 200, 200, 255} : SDL_Color{255, 255, 255, 255};
                        drawRect(ren, b.param2Rect, boxColor2);
                        writeLeftText(ren, font, b.param2, b.param2Rect, {0, 0, 0, 255}, 2);
                    } else {
                        b.param1Rect = {b.rect.x + 6, b.rect.y + 4, 40, b.rect.h - 8};
                        SDL_Color boxColor1 = (activeParamStr == &b.param1) ? SDL_Color{255, 200, 200, 255} : SDL_Color{255, 255, 255, 255};
                        drawRect(ren, b.param1Rect, boxColor1);
                        writeLeftText(ren, font, b.param1, b.param1Rect, {0, 0, 0, 255}, 2);

                        string sign;
                        if (b.customName == "add") sign = "+";
                        else if (b.customName == "subtract") sign = "-";
                        else if (b.customName == "multiply") sign = "*";
                        else sign = "/";

                        SDL_Rect signRect = {b.param1Rect.x + b.param1Rect.w + 6, b.rect.y, 18, b.rect.h};
                        writeCenteredText(ren, font, sign, signRect);

                        b.param2Rect = {signRect.x + signRect.w + 6, b.rect.y + 4, 40, b.rect.h - 8};
                        SDL_Color boxColor2 = (activeParamStr == &b.param2) ? SDL_Color{255, 200, 200, 255} : SDL_Color{255, 255, 255, 255};
                        drawRect(ren, b.param2Rect, boxColor2);
                        writeLeftText(ren, font, b.param2, b.param2Rect, {0, 0, 0, 255}, 2);
                    }
                } else
                    writeCenteredText(ren, font, b.customName, b.rect);
            } else
                writeCenteredText(ren, font, b.customName, b.rect);
        }

        if (dropdownOpen && activeDropdownBlock != nullptr) {
            vector<string> opts = getDropdownOptions(activeDropdownBlock->customName);
            for (size_t j = 0; j < opts.size(); j++) {
                SDL_Rect optRect = {
                        activeDropdownBlock->param1Rect.x,
                        activeDropdownBlock->param1Rect.y + activeDropdownBlock->param1Rect.h + (int)(j * 20),
                        80, 20
                };
                drawRect(ren, optRect, {240, 240, 240, 255});
                SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
                SDL_RenderDrawRect(ren, &optRect);
                writeLeftText(ren, font, opts[j], optRect, {0, 0, 0, 255}, 5);
            }
        }

        SDL_Rect operatorsLabelRect = {720, 520, 260, 20};
        writeLeftText(ren, font, "Operators Result:", operatorsLabelRect, {0, 0, 0, 255}, 0);
        if (!operatorResultText.empty()) {
            SDL_Color resColor = operatorResultError ? SDL_Color{180, 0, 0, 255} : SDL_Color{0, 100, 0, 255};
            SDL_Rect operatorsValueRect = {720, 540, 260, 20};
            writeLeftText(ren, font, operatorResultText, operatorsValueRect, resColor, 0);
        }

        boxRGBA(ren, addExtensionBtn.x, addExtensionBtn.y,
                addExtensionBtn.x + addExtensionBtn.w, addExtensionBtn.y + addExtensionBtn.h,
                100, 150, 200, 255);
        writeCenteredText(ren, font, "Add Extension", addExtensionBtn);

        if (fileMenuOpen) {
            boxRGBA(ren, fileItemNew.x, fileItemNew.y,
                    fileItemNew.x + fileItemNew.w, fileItemNew.y + fileItemNew.h,
                    160, 70, 165, 255);
            boxRGBA(ren, fileItemSave.x, fileItemSave.y,
                    fileItemSave.x + fileItemSave.w, fileItemSave.y + fileItemSave.h,
                    160, 70, 165, 255);
            boxRGBA(ren, fileItemLoad.x, fileItemLoad.y,
                    fileItemLoad.x + fileItemLoad.w, fileItemLoad.y + fileItemLoad.h,
                    160, 70, 165, 255);

            writeLeftText(ren, font, "New Project", fileItemNew, {255, 255, 255, 255}, 10);
            writeLeftText(ren, font, "Save Project", fileItemSave, {255, 255, 255, 255}, 10);
            writeLeftText(ren, font, "Load Project", fileItemLoad, {255, 255, 255, 255}, 10);
        }

        if (marioTexture && marioVisible) {
            SDL_Point center = {marioRect.w / 2, marioRect.h / 2};
            double finalAngle = (double) marioData.angle - 90.0;
            SDL_RendererFlip flip = (costumeIndex == 1) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderCopyEx(ren, marioTexture, NULL, &marioRect, finalAngle, &center, flip);

            if (!currentMessage.empty()) {
                if (!messagePersistent && currentTime > messageTimeout)
                    currentMessage = "";
                else {
                    int bx = marioRect.x + marioRect.w;
                    int by = marioRect.y - 45;
                    if (by < 50) by = 50;
                    if (bx > WINDOW_WIDTH - 110) bx = WINDOW_WIDTH - 110;
                    SDL_Rect bubbleRect = {bx, by, 100, 35};

                    if (isThinking) {
                        drawRect(ren, bubbleRect, {240, 240, 255, 255});
                        SDL_Rect c1 = {bx - 10, by + 25, 10, 10};
                        SDL_Rect c2 = {bx - 20, by + 40, 6, 6};
                        drawRect(ren, c1, {240, 240, 255, 255});
                        drawRect(ren, c2, {240, 240, 255, 255});
                    } else {
                        drawRect(ren, bubbleRect, {255, 255, 255, 255});
                        SDL_Rect tail = {bx - 8, by + 20, 10, 10};
                        drawRect(ren, tail, {255, 255, 255, 255});
                    }
                    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
                    SDL_RenderDrawRect(ren, &bubbleRect);
                    writeCenteredText(ren, font, currentMessage, bubbleRect, {0, 0, 0, 255});
                }
            }
        }

        if (extensionMenuOpen) {
            boxRGBA(ren, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 255, 255, 255, 255);
            boxRGBA(ren, getBackBtn.x, getBackBtn.y,
                    getBackBtn.x + getBackBtn.w, getBackBtn.y + getBackBtn.h,
                    200, 100, 100, 255);
            writeCenteredText(ren, font2, "<- Back", getBackBtn);
            boxRGBA(ren, penBtn.x, penBtn.y,
                    penBtn.x + penBtn.w, penBtn.y + penBtn.h,
                    70, 220, 115, 255);
            writeCenteredText(ren, font2, "Pen", penBtn);

            if (penExtensionAdded) {
                SDL_Rect msgRect = {50, 250, 300, 40};
                TTF_Font *fontError = TTF_OpenFont("calibrib.ttf", 26);
                if (fontError) {
                    writeLeftText(ren, fontError, "Pen already added!", msgRect, {0, 0, 0, 255}, 0);
                    TTF_CloseFont(fontError);
                }
            }
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    TTF_CloseFont(font);
    TTF_CloseFont(font2);
    TTF_CloseFont(font3);
    TTF_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_DestroyTexture(logo);
    SDL_DestroyTexture(marioTexture);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
