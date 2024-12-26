#include "nxsg_text.h"
#include "nxsg_renderer.h"
#include <stdexcept>

nxsg::Text::Text(const nxsg::renderer& renderer, const std::string& fontPath, int fontSize)
    : m_renderer(renderer), m_font(nullptr), m_texture(nullptr), m_dirty(true)
{
    m_font = TTF_OpenFont(fontPath.c_str(), fontSize);
    if (!m_font)
    {
        throw std::runtime_error("Failed to load font: " + fontPath + " " + TTF_GetError());
    }
}

nxsg::Text::~Text()
{
    if (m_texture)
    {
        SDL_DestroyTexture(m_texture);
    }
    if (m_font)
    {
        TTF_CloseFont(m_font);
    }
}

void nxsg::Text::setText(const std::string &text, SDL_Color color)
{
    if (!m_dirty)
        return;

    if (m_texture)
    {
        SDL_DestroyTexture(m_texture);
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(m_font, text.c_str(), color,0);
    if (!surface)
    {
        throw std::runtime_error("Failed to create text surface: " + std::string(TTF_GetError()));
    }

    m_texture = SDL_CreateTextureFromSurface(m_renderer.GetSDLRenderer(), surface);
    SDL_QueryTexture(m_texture, nullptr, nullptr, &m_textRect.w, &m_textRect.h);
    SDL_FreeSurface(surface);

    if (!m_texture)
    {
        throw std::runtime_error("Failed to create text texture: " + std::string(SDL_GetError()));
    }

    m_dirty = false;
}

void nxsg::Text::render(int x, int y)
{
    if (m_texture)
    {
        SDL_Rect dstRect = {x, y, m_textRect.w, m_textRect.h};
        SDL_RenderCopy(m_renderer.GetSDLRenderer(), m_texture, nullptr, &dstRect);
    }
}

SDL_Rect nxsg::Text::getRect() const
{
    return m_textRect;
}

int nxsg::Text::width() const
{
    return m_textRect.w;
}

int nxsg::Text::height() const
{
    return m_textRect.h;
}

void nxsg::Text::markDirty()
{
    m_dirty = true;
}

void nxsg::Text::clearDirty()
{
    m_dirty = false;
}
