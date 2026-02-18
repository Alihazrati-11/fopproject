#include <SDL2/SDL.h>
#include <vector>
#include <iostream>

const int WINDOW_WIDTH  = 1000;
const int WINDOW_HEIGHT = 600;

// ناحیه‌ها
SDL_Rect leftPanel  = { 10, 10, 200, 580 };   // پالت بلاک‌ها
SDL_Rect workspace  = { 220, 10, 470, 580 };  // محیط بلاک‌ها
SDL_Rect stagePanel = { 700, 10, 290, 580 };  // صحنه

// اسپرایت دایره‌ای
int spriteX = 740;
int spriteY = 60;
int spriteR = 30;

// رسم مستطیل با حاشیه
void drawRect(SDL_Renderer* ren, SDL_Rect r, SDL_Color fill, SDL_Color border) {
    SDL_SetRenderDrawColor(ren, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(ren, &r);
}

// رسم دایره
void drawCircle(SDL_Renderer* ren, int cx, int cy, int r, SDL_Color color) {
    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);
    for (int y = -r; y <= r; ++y) {
        for (int x = -r; x <= r; ++x) {
            if (x*x + y*y <= r*r) {
                SDL_RenderDrawPoint(ren, cx + x, cy + y);
            }
        }
    }
}

bool pointInRect(int x, int y, const SDL_Rect& r) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

// اطلاعات نوع بلاک (رنگ و جایگاه در پالت)
struct BlockType {
    SDL_Color color;
    SDL_Color border;
    SDL_Rect  protoRect;
};

struct BlockInstance {
    int typeIndex;
    SDL_Rect rect;
    int lastValidX;
    int lastValidY;
    bool dragging = false;
    int dragOffsetX = 0;
    int dragOffsetY = 0;
};

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow(
        "Scratch - SDL2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN
    );
    if (!win) {
        std::cerr << "Window create failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        std::cerr << "Renderer create failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    // تعریف چند نوع بلاک با رنگ‌های مشابه Scratch
    std::vector<BlockType> blockTypes = {
        // Motion
        { {  76,151,255,255}, { 59,131,230,255}, { 30,  50, 160, 40 } },
        // Looks
        { { 153,102,255,255}, {130, 80,230,255}, { 30, 100, 160, 40 } },
        // Sound
        { { 207, 99,207,255}, {180, 80,180,255}, { 30, 150, 160, 40 } },
        // Events
        { { 255,213,  0,255}, {220,190,  0,255}, { 30, 200, 160, 40 } },
        // Control
        { { 255,171, 25,255}, {230,150, 20,255}, { 30, 250, 160, 40 } },
        // Sensing
        { {  92,177,214,255}, { 70,150,190,255}, { 30, 300, 160, 40 } },
        // Operators
        { {  89,192, 89,255}, { 70,165, 70,255}, { 30, 350, 160, 40 } },
        // Variables
        { { 255,140, 26,255}, {230,120, 20,255}, { 30, 400, 160, 40 } }
    };

    std::vector<BlockInstance> instances;

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            // کلیک ماوس
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x;
                int my = e.button.y;

                // اول بررسی بلوک‌های داخل Workspace (از بالا به پایین)
                bool picked = false;
                for (int i = (int)instances.size() - 1; i >= 0; --i) {
                    if (pointInRect(mx, my, instances[i].rect)) {
                        // این بلاک انتخاب شد
                        instances[i].dragging = true;
                        instances[i].dragOffsetX = mx - instances[i].rect.x;
                        instances[i].dragOffsetY = my - instances[i].rect.y;

                        // برای جلو آمدن در رندر، به انتهای لیست منتقل شود
                        BlockInstance temp = instances[i];
                        instances.erase(instances.begin() + i);
                        instances.push_back(temp);
                        picked = true;
                        break;
                    }
                }

                if (!picked) {
                    // اگر روی پالت کلیک شد، یک کپی بساز
                    for (int i = 0; i < (int)blockTypes.size(); ++i) {
                        if (pointInRect(mx, my, blockTypes[i].protoRect)) {
                            BlockInstance b;
                            b.typeIndex = i;
                            b.rect = blockTypes[i].protoRect;
                            b.dragging = true;
                            b.dragOffsetX = mx - b.rect.x;
                            b.dragOffsetY = my - b.rect.y;
                            b.lastValidX = b.rect.x;
                            b.lastValidY = b.rect.y;
                            instances.push_back(b);
                            break;
                        }
                    }
                }
            }

            // درگ
            if (e.type == SDL_MOUSEMOTION) {
                int mx = e.motion.x;
                int my = e.motion.y;
                if (!instances.empty()) {
                    BlockInstance& b = instances.back();
                    if (b.dragging) {
                        b.rect.x = mx - b.dragOffsetX;
                        b.rect.y = my - b.dragOffsetY;
                    }
                }
            }

            // رها کردن
            if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                if (!instances.empty()) {
                    BlockInstance& b = instances.back();
                    if (b.dragging) {
                        b.dragging = false;

                        int cx = b.rect.x + b.rect.w / 2;
                        int cy = b.rect.y + b.rect.h / 2;

                        if (pointInRect(cx, cy, workspace)) {
                            // داخل Workspace: بماند و موقعیت معتبر ذخیره شود
                            b.lastValidX = b.rect.x;
                            b.lastValidY = b.rect.y;

                            // حرکت تستی اسپرایت
                            spriteX += 10;
                            if (spriteX + spriteR > stagePanel.x + stagePanel.w - 10)
                                spriteX = stagePanel.x + 10 + spriteR;
                        }
                        else if (pointInRect(cx, cy, stagePanel)) {
                            // داخل Stage: برگردد به آخرین موقعیت معتبر
                            b.rect.x = b.lastValidX;
                            b.rect.y = b.lastValidY;

                            // اگر هنوز هیچ‌وقت وارد Workspace نشده، حذف شود (مثل Scratch)
                            if (!pointInRect(b.rect.x + b.rect.w/2, b.rect.y + b.rect.h/2, workspace)) {
                                instances.pop_back();
                            }
                        }
                        else {
                            // خارج از Workspace: حذف (مثل Scratch)
                            instances.pop_back();
                        }
                    }
                }
            }
        }

        // پس‌زمینه
        SDL_SetRenderDrawColor(ren, 246, 247, 251, 255);
        SDL_RenderClear(ren);

        // پنل‌ها
        drawRect(ren, leftPanel,  {255,255,255,255}, {226,230,239,255});
        drawRect(ren, workspace,  {248,249,255,255}, {226,230,239,255});
        drawRect(ren, stagePanel, {240,244,255,255}, {226,230,239,255});

        // بلوک‌های پالت
        for (auto& bt : blockTypes) {
            drawRect(ren, bt.protoRect, bt.color, bt.border);
        }

        // بلوک‌های داخل Workspace
        for (auto& b : instances) {
            auto& bt = blockTypes[b.typeIndex];
            drawRect(ren, b.rect, bt.color, bt.border);
        }

        // اسپرایت
        drawCircle(ren, spriteX, spriteY, spriteR, {255,159,28,255});

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
