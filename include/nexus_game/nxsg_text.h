#ifndef NXSG_TEXT_H
#define NXSG_TEXT_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>

namespace nxsg {
class renderer;
class Text {
public:
    Text(const nxsg::renderer& renderer, const std::string& fontPath, int fontSize);
    ~Text();

    void setText(const std::string& text, SDL_Color color);
    void render(int x, int y);

    SDL_Rect getRect() const;
    int width() const;
    int height() const;
    void markDirty();
    void clearDirty();

private:
    bool m_dirty{true};
    const nxsg::renderer& m_renderer;
    TTF_Font* m_font;
    SDL_Texture* m_texture;
    SDL_Rect m_textRect;
};

} // namespace nxsg

#endif // NXSG_TEXT_H