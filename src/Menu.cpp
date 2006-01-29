/* GG is a GUI for SDL and OpenGL.
   Copyright (C) 2003 T. Zachary Laine

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1
   of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
    
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA

   If you do not wish to comply with the terms of the LGPL please
   contact the author as other terms are available for a fee.
    
   Zach Laine
   whatwasthataddress@hotmail.com */

/* $Header$ */

#include <GG/Menu.h>

#include <GG/GUI.h>
#include <GG/DrawUtil.h>
#include <GG/StyleFactory.h>
#include <GG/TextControl.h>
#include <GG/WndEditor.h>

using namespace GG;

namespace {
    const int BORDER_THICKNESS = 1; // thickness with which to draw menu borders
    const int MENU_SEPARATION = 10; // distance between menu texts in a MenuBar, in pixels
}

struct GG::SetFontAction : AttributeChangedAction<boost::shared_ptr<Font> >
{
    SetFontAction(MenuBar* menu_bar) : m_menu_bar(menu_bar) {}
    void operator()(const boost::shared_ptr<Font>&) {m_menu_bar->AdjustLayout(true);}
private:
    MenuBar* m_menu_bar;
};

struct GG::SetTextColorAction : AttributeChangedAction<Clr>
{
    SetTextColorAction(MenuBar* menu_bar) : m_menu_bar(menu_bar) {}
    void operator()(const Clr&) {m_menu_bar->AdjustLayout(true);}
private:
    MenuBar* m_menu_bar;
};


////////////////////////////////////////////////
// GG::MenuItem
////////////////////////////////////////////////
MenuItem::MenuItem() : 
    SelectedIDSignal(new SelectedIDSignalType()),
    SelectedSignal(new SelectedSignalType()),
    item_ID(0), 
    disabled(false), 
    checked(false)
{}

MenuItem::MenuItem(const std::string& str, int id, bool disable, bool check) : 
    SelectedIDSignal(new SelectedIDSignalType()),
    SelectedSignal(new SelectedSignalType()),
    label(str), 
    item_ID(id), 
    disabled(disable), 
    checked(check)
{}

MenuItem::MenuItem(const std::string& str, int id, bool disable, bool check, const SelectedIDSlotType& slot) : 
    SelectedIDSignal(new SelectedIDSignalType()),
    SelectedSignal(new SelectedSignalType()),
    label(str), 
    item_ID(id), 
    disabled(disable), 
    checked(check)
{
    SelectedIDSignal->connect(slot);
}

MenuItem::MenuItem(const std::string& str, int id, bool disable, bool check, const SelectedSlotType& slot) : 
    SelectedIDSignal(new SelectedIDSignalType()),
    SelectedSignal(new SelectedSignalType()),
    label(str), 
    item_ID(id), 
    disabled(disable), 
    checked(check)
{
    SelectedSignal->connect(slot);
}

MenuItem::~MenuItem()
{}


////////////////////////////////////////////////
// GG::MenuBar
////////////////////////////////////////////////
MenuBar::MenuBar() :
    Control(),
    m_caret(-1)
{}

MenuBar::MenuBar(int x, int y, int w, const boost::shared_ptr<Font>& font, Clr text_color/* = CLR_WHITE*/,
                 Clr color/* = CLR_BLACK*/, Clr interior/* = CLR_SHADOW*/) :
    Control(x, y, w, font->Lineskip()),
    m_font(font),
    m_border_color(color),
    m_int_color(interior),
    m_text_color(text_color),
    m_sel_text_color(text_color),
    m_caret(-1)
{
    // use opaque interior color as hilite color
    interior.a = 255;
    m_hilite_color = interior;
    AdjustLayout();
}

MenuBar::MenuBar(int x, int y, int w, const boost::shared_ptr<Font>& font, const MenuItem& m,
                 Clr text_color/* = CLR_WHITE*/, Clr color/* = CLR_BLACK*/, Clr interior/* = CLR_SHADOW*/) :
    Control(x, y, w, font->Lineskip()),
    m_font(font),
    m_border_color(color),
    m_int_color(interior),
    m_text_color(text_color),
    m_sel_text_color(text_color),
    m_menu_data(m),
    m_caret(-1)
{
    // use opaque interior color as hilite color
    interior.a = 255;
    m_hilite_color = interior;
    AdjustLayout();
}

const MenuItem& MenuBar::AllMenus() const
{
    return m_menu_data;
}

bool MenuBar::ContainsMenu(const std::string& str) const
{
    bool retval = false;
    for (std::vector<MenuItem>::const_iterator it = m_menu_data.next_level.begin(); it != m_menu_data.next_level.end(); ++it) {
        if (it->label == str) {
            retval = true;
            break;
        }
    }
    return retval;
}

int MenuBar::NumMenus() const
{
    return m_menu_data.next_level.size();
}

const MenuItem& MenuBar::GetMenu(const std::string& str) const
{
    std::vector<MenuItem>::const_iterator it = m_menu_data.next_level.begin();
    for (; it != m_menu_data.next_level.end(); ++it) {
        if (it->label == str)
            break;
    }
    return *it;
}

const MenuItem& MenuBar::GetMenu(int n) const
{
    return *(m_menu_data.next_level.begin() + n);
}

Clr MenuBar::BorderColor() const        
{
    return m_border_color;
}

Clr MenuBar::InteriorColor() const
{
    return m_int_color;
}

Clr MenuBar::TextColor() const
{
    return m_text_color;
}

Clr MenuBar::HiliteColor() const
{
    return m_hilite_color;
}

Clr MenuBar::SelectedTextColor() const
{
    return m_sel_text_color;
}

void MenuBar::Render()
{
    Pt ul = UpperLeft();
    Pt lr = LowerRight();
    FlatRectangle(ul.x, ul.y, lr.x, lr.y, m_int_color, m_border_color, BORDER_THICKNESS);

    // paint caret, if any
    if (m_caret != -1) {
        Pt caret_ul = m_menu_labels[m_caret]->UpperLeft() + Pt((!m_caret ? BORDER_THICKNESS : 0), BORDER_THICKNESS);
        Pt caret_lr = m_menu_labels[m_caret]->LowerRight() - Pt((m_caret == static_cast<int>(m_menu_labels.size()) - 1 ? BORDER_THICKNESS : 0), BORDER_THICKNESS);
        FlatRectangle(caret_ul.x, caret_ul.y, caret_lr.x, caret_lr.y, m_hilite_color, CLR_ZERO, 0);
    }
}

void MenuBar::LButtonDown(const Pt& pt, Uint32 keys)
{
    if (!Disabled()) {
        for (int i = 0; i < static_cast<int>(m_menu_labels.size()); ++i) {
            if (m_menu_labels[i]->InWindow(pt)) {
                m_caret = -1;
                BrowsedSignal(0);
                // since a MenuBar is usually modeless, but becomes modal when a menu is opened, we do something kludgey here:
                // we launch a PopupMenu whenever a menu item is selected, then use the ID returned from it to find the
                // menu item that was chosen; we then emit a signal from that item
                if (m_menu_data.next_level[i].next_level.empty()) {
                    (*m_menu_data.next_level[i].SelectedIDSignal)(m_menu_data.next_level[i].item_ID);
                    (*m_menu_data.next_level[i].SelectedSignal)();
                } else {
                    MenuItem popup_data;
                    PopupMenu menu(m_menu_labels[i]->UpperLeft().x, m_menu_labels[i]->LowerRight().y, m_font, m_menu_data.next_level[i], m_text_color, m_border_color, m_int_color);
                    menu.SetHiliteColor(m_hilite_color);
                    menu.SetSelectedTextColor(m_sel_text_color);
                    Connect(menu.BrowsedSignal, &MenuBar::BrowsedSlot, this);
                    menu.Run();
                }
            }
        }
    }
}

void MenuBar::MouseHere(const Pt& pt, Uint32 keys)
{
    if (!Disabled()) {
        m_caret = -1;
        for (unsigned int i = 0; i < m_menu_data.next_level.size(); ++i) {
            if (m_menu_labels[i]->InWindow(pt)) {
                m_caret = i;
                break;
            }
        }
    }
}

void MenuBar::MouseLeave(const Pt& pt, Uint32 keys)
{
    m_caret = -1;
}

void MenuBar::SizeMove(const Pt& ul, const Pt& lr)
{
    Wnd::SizeMove(ul, lr);
    AdjustLayout();
}

MenuItem& MenuBar::AllMenus()
{
    return m_menu_data;
}

MenuItem& MenuBar::GetMenu(const std::string& str)
{
    std::vector<MenuItem>::iterator it = m_menu_data.next_level.begin();
    for (; it != m_menu_data.next_level.end(); ++it) {
        if (it->label == str)
            break;
    }
    return *it;
}

MenuItem& MenuBar::GetMenu(int n)
{
    return m_menu_data.next_level[n];
}

void MenuBar::AddMenu(const MenuItem& menu)
{
    m_menu_data.next_level.push_back(menu);
    AdjustLayout();
}

void MenuBar::SetBorderColor(Clr clr)
{
    m_border_color = clr;
}

void MenuBar::SetInteriorColor(Clr clr)
{
    m_int_color = clr;
}

void MenuBar::SetTextColor(Clr clr)
{
    m_text_color = clr;
}

void MenuBar::SetHiliteColor(Clr clr)
{
    m_hilite_color = clr;
}

void MenuBar::SetSelectedTextColor(Clr clr)
{
    m_sel_text_color = clr;
}

void MenuBar::DefineAttributes(WndEditor* editor)
{
    if (!editor)
        return;
    Control::DefineAttributes(editor);
    editor->Label("MenuBar");
    boost::shared_ptr<SetFontAction> set_font_action(new SetFontAction(this));
    editor->Attribute<boost::shared_ptr<Font> >("Font", m_font, set_font_action);
    editor->Attribute("Border Color", m_border_color);
    editor->Attribute("Interior Color", m_int_color);
    boost::shared_ptr<SetTextColorAction> set_text_color_action(new SetTextColorAction(this));
    editor->Attribute<Clr>("Text Color", m_text_color, set_text_color_action);
    editor->Attribute("Highlighting Color", m_hilite_color);
    editor->Attribute("Selected Text Color", m_sel_text_color);
    // TODO: handle assigning menu items
}

const boost::shared_ptr<Font>& MenuBar::GetFont() const
{
    return m_font;
}

const std::vector<TextControl*>& MenuBar::MenuLabels() const
{
    return m_menu_labels;
}

int MenuBar::Caret() const
{
    return m_caret;
}

void MenuBar::AdjustLayout(bool reset/* = false*/)
{
    if (reset) {
        DeleteChildren();
        m_menu_labels.clear();
    }

    // create any needed labels
    for (unsigned int i = m_menu_labels.size(); i < m_menu_data.next_level.size(); ++i) {
        m_menu_labels.push_back(GetStyleFactory()->NewTextControl(0, 0, m_menu_data.next_level[i].label, m_font, m_text_color));
        m_menu_labels.back()->Resize(Pt(m_menu_labels.back()->Width() + 2 * MENU_SEPARATION, m_font->Lineskip()));
        AttachChild(m_menu_labels.back());
    }

    // determine rows layout
    std::vector<int> menu_rows; // each element is the last + 1 index displayable on that row
    int space = Width();
    for (unsigned int i = 0; i < m_menu_labels.size(); ++i) {
        space -= m_menu_labels[i]->Width();
        if (space < 0) { // if this menu's text won't fit in the available space
            space = Width();
            // if moving this menu to the next row would leave an empty row, leave it here even though it won't quite fit
            if (!menu_rows.empty() && menu_rows.back() == static_cast<int>(i) - 1) {
                menu_rows.push_back(i + 1);
            } else {
                menu_rows.push_back(i);
                space -= m_menu_labels[i]->Width();
            }
        }
    }
    if (menu_rows.empty() || menu_rows.back() < static_cast<int>(m_menu_labels.size()))
        menu_rows.push_back(m_menu_labels.size());

    // place labels
    int label_i = 0;
    for (unsigned int row = 0; row < menu_rows.size(); ++row) {
        int x = 0;
        for (; label_i < menu_rows[row]; ++label_i) {
            m_menu_labels[label_i]->MoveTo(Pt(x, row * m_font->Lineskip()));
            x += m_menu_labels[label_i]->Width();
        }
    }

    // resize MenuBar if needed
    int desired_ht = std::max(size_t(1), menu_rows.size()) * m_font->Lineskip();
    if (Height() != desired_ht)
        Resize(Pt(Width(), desired_ht));
}

void MenuBar::BrowsedSlot(int n)
{
    BrowsedSignal(n);
}


////////////////////////////////////////////////
// GG::PopupMenu
////////////////////////////////////////////////
namespace {
const int HORIZONTAL_MARGIN = 3; // distance to leave between edge of PopupMenu contents and the control's border
}

PopupMenu::PopupMenu(int x, int y, const boost::shared_ptr<Font>& font, const MenuItem& m, Clr text_color/* = CLR_WHITE*/,
                     Clr color/* = CLR_BLACK*/, Clr interior/* = CLR_SHADOW*/) :
    Wnd(0, 0, GUI::GetGUI()->AppWidth() - 1, GUI::GetGUI()->AppHeight() - 1, CLICKABLE | MODAL),
    m_font(font),
    m_border_color(color),
    m_int_color(interior),
    m_text_color(text_color),
    m_sel_text_color(text_color),
    m_menu_data(m),
    m_open_levels(),
    m_caret(std::vector<int>(1, -1)),
    m_origin(Pt(x, y)),
    m_item_selected(0)
{
    // use opaque interior color as hilite color
    interior.a = 255;
    m_hilite_color = interior;
    m_open_levels.resize(1);
}

Pt PopupMenu::ClientUpperLeft() const
{
    return m_origin;
}

int PopupMenu::MenuID() const
{
    return (m_item_selected ? m_item_selected->item_ID : 0);
}

Clr PopupMenu::BorderColor() const
{
    return m_border_color;
}

Clr PopupMenu::InteriorColor() const
{
    return m_int_color;
}

Clr PopupMenu::TextColor() const
{
    return m_text_color;
}

Clr PopupMenu::HiliteColor() const
{
    return m_hilite_color;
}

Clr PopupMenu::SelectedTextColor() const
{
    return m_sel_text_color;
}

void PopupMenu::Render()
{
    if (m_menu_data.next_level.size())
    {
        Pt ul = ClientUpperLeft();

        const int INDICATOR_VERTICAL_MARGIN = 3;
        const int INDICATOR_HEIGHT = m_font->Lineskip() - 2 * INDICATOR_VERTICAL_MARGIN;
        const int CHECK_HEIGHT = INDICATOR_HEIGHT;
        const int CHECK_WIDTH = CHECK_HEIGHT;

        int next_menu_x_offset = 0;
        int next_menu_y_offset = 0;
        for (unsigned int i = 0; i < m_caret.size(); ++i) {
            bool needs_indicator = false;

            // get the correct submenu
            MenuItem* menu_ptr = &m_menu_data;
            for (unsigned int j = 0; j < i; ++j)
                menu_ptr = &menu_ptr->next_level[m_caret[j]];
            MenuItem& menu = *menu_ptr;

            // determine the total size of the menu, render it, and record its bounding rect
            std::string str;
            for (unsigned int j = 0; j < menu.next_level.size(); ++j) {
                str += menu.next_level[j].label + (static_cast<int>(j) < static_cast<int>(menu.next_level.size()) - 1 ? "\n" : "");
                if (menu.next_level[j].next_level.size() || menu.next_level[j].checked)
                    needs_indicator = true;
            }
            std::vector<Font::LineData> lines;
            Uint32 fmt = TF_LEFT | TF_TOP;
            Pt menu_sz = m_font->DetermineLines(str, fmt, 0, lines); // get dimensions of text in menu
            menu_sz.x += 2 * HORIZONTAL_MARGIN;
            if (needs_indicator)
                menu_sz.x += CHECK_WIDTH + 2 * HORIZONTAL_MARGIN; // make room for the little arrow
            Rect r(ul.x + next_menu_x_offset, ul.y + next_menu_y_offset,
                   ul.x + next_menu_x_offset + menu_sz.x, ul.y + next_menu_y_offset + menu_sz.y);

            if (r.lr.x > GUI::GetGUI()->AppWidth()) {
                int offset = r.lr.x - GUI::GetGUI()->AppWidth();
                r.ul.x -= offset;
                r.lr.x -= offset;
            }
            if (r.lr.y > GUI::GetGUI()->AppHeight()) {
                int offset = r.lr.y - GUI::GetGUI()->AppHeight();
                r.ul.y -= offset;
                r.lr.y -= offset;
            }
            next_menu_x_offset = menu_sz.x;
            next_menu_y_offset = m_caret[i] * m_font->Lineskip();
            FlatRectangle(r.ul.x, r.ul.y, r.lr.x, r.lr.y, m_int_color, m_border_color, BORDER_THICKNESS);
            m_open_levels[i] = r;

            // paint caret, if any
            if (m_caret[i] != -1) {
                Rect tmp_r = r;
                tmp_r.ul.y += m_caret[i] * m_font->Lineskip();
                tmp_r.lr.y = tmp_r.ul.y + m_font->Lineskip() + 3;
                tmp_r.ul.x += BORDER_THICKNESS;
                tmp_r.lr.x -= BORDER_THICKNESS;
                if (m_caret[i] == 0)
                    tmp_r.ul.y += BORDER_THICKNESS;
                if (m_caret[i] == static_cast<int>(menu.next_level.size()) - 1)
                    tmp_r.lr.y -= BORDER_THICKNESS;
                FlatRectangle(tmp_r.ul.x, tmp_r.ul.y, tmp_r.lr.x, tmp_r.lr.y, m_hilite_color, CLR_ZERO, 0);
            }

            // paint menu text and submenu indicator arrows
            Rect line_rect = r;
            line_rect.ul.x += HORIZONTAL_MARGIN;
            line_rect.lr.x -= HORIZONTAL_MARGIN;
            for (unsigned int j = 0; j < menu.next_level.size(); ++j) {
                Clr clr = (m_caret[i] == static_cast<int>(j)) ?
                          (menu.next_level[j].disabled ? DisabledColor(m_sel_text_color) : m_sel_text_color) :
                                  (menu.next_level[j].disabled ? DisabledColor(m_text_color) : m_text_color);
                glColor3ubv(clr.v);
                m_font->RenderText(line_rect.ul.x, line_rect.ul.y, line_rect.lr.x, line_rect.lr.y, menu.next_level[j].label, fmt);
                if (menu.next_level[j].checked) {
                    FlatCheck(line_rect.lr.x - CHECK_WIDTH - HORIZONTAL_MARGIN, line_rect.ul.y + INDICATOR_VERTICAL_MARGIN,
                              line_rect.lr.x - HORIZONTAL_MARGIN, line_rect.ul.y + INDICATOR_VERTICAL_MARGIN + CHECK_HEIGHT, clr);
                }
                if (menu.next_level[j].next_level.size()) {
                    glDisable(GL_TEXTURE_2D);
                    glBegin(GL_TRIANGLES);
                    glVertex2d(line_rect.lr.x - INDICATOR_HEIGHT / 2.0 - HORIZONTAL_MARGIN, line_rect.ul.y + INDICATOR_VERTICAL_MARGIN);
                    glVertex2d(line_rect.lr.x - INDICATOR_HEIGHT / 2.0 - HORIZONTAL_MARGIN, line_rect.ul.y + m_font->Lineskip() - INDICATOR_VERTICAL_MARGIN);
                    glVertex2d(line_rect.lr.x - HORIZONTAL_MARGIN,                          line_rect.ul.y + m_font->Lineskip() / 2.0);
                    glEnd();
                    glEnable(GL_TEXTURE_2D);
                }
                line_rect.ul.y += m_font->Lineskip();
            }
        }
    }
}

void PopupMenu::LButtonUp(const Pt& pt, Uint32 keys)
{
    if (m_caret[0] != -1) {
        MenuItem* menu_ptr = &m_menu_data;
        for (unsigned int i = 0; i < m_caret.size(); ++i)
            menu_ptr = &menu_ptr->next_level[m_caret[i]];
        if (!menu_ptr->disabled) {
            m_item_selected = menu_ptr;
        }
    }
    BrowsedSignal(0);
    m_done = true;
}

void PopupMenu::LClick(const Pt& pt, Uint32 keys)
{
    LButtonUp(pt, keys);
}

void PopupMenu::LDrag(const Pt& pt, const Pt& move, Uint32 keys)
{
    bool cursor_is_in_menu = false;
    for (int i = static_cast<int>(m_open_levels.size()) - 1; i >= 0; --i) {
        // get the correct submenu
        MenuItem* menu_ptr = &m_menu_data;
        for (int j = 0; j < i; ++j)
            menu_ptr = &menu_ptr->next_level[m_caret[j]];
        MenuItem& menu = *menu_ptr;

        if (pt.x >= m_open_levels[i].ul.x && pt.x <= m_open_levels[i].lr.x &&
                pt.y >= m_open_levels[i].ul.y && pt.y <= m_open_levels[i].lr.y) {
            int row_selected = (pt.y - m_open_levels[i].ul.y) / m_font->Lineskip();
            if (row_selected == m_caret[i]) {
                cursor_is_in_menu = true;
            } else if (row_selected >= 0 && row_selected < static_cast<int>(menu.next_level.size())) {
                m_caret[i] = row_selected;
                m_open_levels.resize(i + 1);
                m_caret.resize(i + 1);
                if (!menu.next_level[row_selected].disabled && menu.next_level[row_selected].next_level.size()) {
                    m_caret.push_back(0);
                    m_open_levels.push_back(Rect());
                }
                cursor_is_in_menu = true;
            }
        }
    }
    if (!cursor_is_in_menu) {
        m_open_levels.resize(1);
        m_caret.resize(1);
        m_caret[0] = -1;
    }
    int update_ID = 0;
    if (m_caret[0] != -1) {
        MenuItem* menu_ptr = &m_menu_data;
        for (unsigned int i = 0; i < m_caret.size(); ++i)
            menu_ptr = &menu_ptr->next_level[m_caret[i]];
        update_ID = menu_ptr->item_ID;
    }
    BrowsedSignal(update_ID);
}

void PopupMenu::RButtonUp(const Pt& pt, Uint32 keys)
{
    LButtonUp(pt, keys);
}

void PopupMenu::RClick(const Pt& pt, Uint32 keys)
{
    LButtonUp(pt, keys);
}

void PopupMenu::MouseHere(const Pt& pt, Uint32 keys)
{
    LDrag(pt, Pt(), keys);
}

int PopupMenu::Run()
{
    int retval = Wnd::Run();
    if (m_item_selected) {
        (*m_item_selected->SelectedIDSignal)(m_item_selected->item_ID);
        (*m_item_selected->SelectedSignal)();
    }
    return retval;
}

void PopupMenu::SetBorderColor(Clr clr)
{
    m_border_color = clr;
}

void PopupMenu::SetInteriorColor(Clr clr)     
{
    m_int_color = clr;
}

void PopupMenu::SetTextColor(Clr clr)         
{
    m_text_color = clr;
}

void PopupMenu::SetHiliteColor(Clr clr)       
{
    m_hilite_color = clr;
}

void PopupMenu::SetSelectedTextColor(Clr clr)
{
    m_sel_text_color = clr;
}

const boost::shared_ptr<Font>& PopupMenu::GetFont() const     
{
    return m_font;
}

const MenuItem& PopupMenu::MenuData() const    
{
    return m_menu_data;
}

const std::vector<Rect>& PopupMenu::OpenLevels() const  
{
    return m_open_levels;
}

const std::vector<int>& PopupMenu::Caret() const       
{
    return m_caret;
}

const MenuItem* PopupMenu::ItemSelected() const
{
    return m_item_selected;
}