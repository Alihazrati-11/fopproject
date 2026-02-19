#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfx.h>
#include <vector>
#include <iostream>
using namespace std;
const int WINDOW_WIDTH  = 1000, WINDOW_HEIGHT = 600;

// ناحیه‌ها
SDL_Rect leftPanel  = { 10, 40, 200, 550 };   // پالت بلاک‌ها
SDL_Rect workspace  = { 220, 40, 470, 550 };  // محیط بلاک‌ها
SDL_Rect stagePanel = { 700, 40, 290, 550 };  // صحنه

// اسپرایت دایره‌ای
int spriteX = 845, spriteY = 315, spriteR = 20;

// رسم مستطیل با حاشیه
void drawRect(SDL_Renderer* ren, SDL_Rect r, SDL_Color fill) {
    SDL_SetRenderDrawColor(ren, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(ren, &r);
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
    SDL_Rect  protoRect;
};

struct BlockInstance {
    int typeIndex;
    SDL_Rect rect;
    int lastValidX, lastValidY;
    bool dragging = false;
    int dragStartX = 0, dragStartY = 0;
};

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win = SDL_CreateWindow(
        "main window",SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    // تعریف چند نوع بلاک با رنگ‌های مشابه Scratch
    vector <BlockType> blockTypes = {
        // Motion
        { {  70,150,255,255}, { 30,  50, 160, 40 } },
        // Looks
        { { 150,100,255,255}, { 30, 100, 160, 40 } },
        // Sound
        { { 210, 100,210,255}, { 30, 150, 160, 40 } },
        // Events
        { { 255,210,  0,255}, { 30, 200, 160, 40 } },
        // Control
        { { 255,170, 25,255}, { 30, 250, 160, 40 } },
        // Sensing
        { {  90,180,210,255}, { 30, 300, 160, 40 } },
        // Operators
        { {  90,190, 90,255}, { 30, 350, 160, 40 } },
        // Variables
        { { 255,140, 25,255}, { 30, 400, 160, 40 } }
    };

    vector <BlockInstance> instances;

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            // کلیک ماوس
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;

                // اول بررسی بلوک‌های داخل Workspace (از بالا به پایین)
                bool picked = false;
                for (int i = instances.size() - 1; i >= 0; --i) {
                    if (pointInRect(mx, my, instances[i].rect)) {
                        // این بلاک انتخاب شد
                        instances[i].dragging = true;
                        instances[i].dragStartX = mx - instances[i].rect.x;
                        instances[i].dragStartY = my - instances[i].rect.y;
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
                    for (int i = 0; i < blockTypes.size(); ++i) {
                        if (pointInRect(mx, my, blockTypes[i].protoRect)) {
                            BlockInstance b;
                            b.typeIndex = i;
                            b.rect = blockTypes[i].protoRect;
                            b.dragging = true;
                            b.dragStartX = mx - b.rect.x;
                            b.dragStartY = my - b.rect.y;
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
                int mx = e.motion.x, my = e.motion.y;
                if (!instances.empty()) {
                    BlockInstance& b = instances.back();
                    if (b.dragging) {
                        b.rect.x = mx - b.dragStartX;
                        b.rect.y = my - b.dragStartY;
                    }
                }
            }

            // رها کردن
            if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT&&!instances.empty()) {
                BlockInstance &b = instances.back();
                if (b.dragging) {
                    b.dragging = false;
                    int cx = b.rect.x + b.rect.w / 2, cy = b.rect.y + b.rect.h / 2;

                    if (pointInRect(cx, cy, workspace)) {
                        // داخل Workspace: بماند و موقعیت معتبر ذخیره شود
                        b.lastValidX = b.rect.x;
                        b.lastValidY = b.rect.y;
                    }else if (pointInRect(cx, cy, stagePanel)) {
                        // داخل Stage: برگردد به آخرین موقعیت معتبر
                        b.rect.x = b.lastValidX;
                        b.rect.y = b.lastValidY;
                        // اگر هنوز هیچ‌وقت وارد Workspace نشده، حذف شود (مثل Scratch)
                        if (!pointInRect(b.rect.x + b.rect.w/2, b.rect.y + b.rect.h/2, workspace))
                            instances.pop_back();
                    }else
                        // خارج از Workspace: حذف (مثل Scratch)
                        instances.pop_back();
                }
            }
        }

        // پس‌زمینه
        SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
        SDL_RenderClear(ren);

        //منوی فایل ولوگو
        boxRGBA(ren, 0, 0, WINDOW_WIDTH, 38, 160, 70, 165, 200);

        // پنل‌ها
        drawRect(ren, leftPanel,  {255,255,255,255});
        drawRect(ren, workspace,  {248,249,255,255});
        drawRect(ren, stagePanel, {240,244,255,255});

        // بلوک‌های پالت
        for (auto& bt : blockTypes)
            drawRect(ren, bt.protoRect, bt.color);

        // بلوک‌های داخل Workspace
        for (auto& b : instances) {
            auto& bt = blockTypes[b.typeIndex];
            drawRect(ren, b.rect, bt.color);
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
