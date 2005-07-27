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

#include "GGLayout.h"

#include <GGWndEditor.h>

#include <cassert>
#include <cmath>

namespace {
    int MinDueToMargin(int cell_margin, int num_rows_or_columns, int row_or_column)
    {
        return (row_or_column == 0 || row_or_column == num_rows_or_columns - 1) ?
            static_cast<int>(std::ceil(cell_margin / 2.0)) :
            cell_margin;
    }
}

using namespace GG;

// RowColParams
Layout::RowColParams::RowColParams() :
    stretch(0), min(0), effective_min(0)
{}

// WndPosition
Layout::WndPosition::WndPosition() :
    first_row(0), first_column(0), last_row(0), last_column(0), alignment(0)
{}

Layout::WndPosition::WndPosition(int first_row_, int first_column_, int last_row_, int last_column_, Uint32 alignment_, const Pt& original_size_) :
    first_row(first_row_), first_column(first_column_), last_row(last_row_), last_column(last_column_), alignment(alignment_), original_size(original_size_)
{}

// Layout
Layout::Layout() :
    Wnd()
{
}

Layout::Layout(int rows, int columns, int border_margin/* = 0*/, int cell_margin/* = -1*/) :
    Wnd(0, 0, 1, 1, 0),
    m_cells(rows, std::vector<Wnd*>(columns)),
    m_border_margin(border_margin),
    m_cell_margin(cell_margin < 0 ? border_margin : cell_margin),
    m_row_params(rows),
    m_column_params(columns),
    m_ignore_child_resize(false)
{
    if (m_border_margin < 0)
        throw std::invalid_argument("Layout::Layout() : m_border_margin may not be less than 0");
}

Layout::Layout(int x, int y, int w, int h, int rows, int columns, int border_margin/* = 0*/, int cell_margin/* = -1*/) :
    Wnd(x, y, w, h, 0),
    m_cells(rows, std::vector<Wnd*>(columns)),
    m_border_margin(border_margin),
    m_cell_margin(cell_margin < 0 ? border_margin : cell_margin),
    m_row_params(rows),
    m_column_params(columns),
    m_ignore_child_resize(false)
{
    if (m_border_margin < 0)
        throw std::invalid_argument("Layout::Layout() : m_border_margin may not be less than 0");
}

int Layout::Rows() const
{
    return m_cells.size();
}

int Layout::Columns() const
{
    return m_cells.empty() ? 0 : m_cells[0].size();
}

Uint32 Layout::ChildAlignment(Wnd* wnd) const
{
    std::map<Wnd*, WndPosition>::const_iterator it = m_wnd_positions.find(wnd);
    if (it == m_wnd_positions.end())
        throw std::runtime_error("Layout::ChildAlignment() : Alignment of a nonexistent child was requested");
    return it->second.alignment;
}

int Layout::BorderMargin() const
{
    return m_border_margin;
}

int Layout::CellMargin() const
{
    return m_cell_margin;
}

double Layout::RowStretch(int row) const
{
    return m_row_params[row].stretch;
}

double Layout::ColumnStretch(int column) const
{
    return m_column_params[column].stretch;
}

int Layout::MinimumRowHeight(int row) const
{
    return m_row_params[row].min;
}

int Layout::MinimumColumnWidth(int column) const
{
    return m_column_params[column].min;
}

void Layout::SizeMove(int x1, int y1, int x2, int y2)
{
    // reset effective_min values
    for (unsigned int i = 0; i < m_row_params.size(); ++i) {
        m_row_params[i].effective_min = std::max(m_row_params[i].min, MinDueToMargin(m_cell_margin, m_row_params.size(), i));
    }

    for (unsigned int i = 0; i < m_column_params.size(); ++i) {
        m_column_params[i].effective_min = std::max(m_column_params[i].min, MinDueToMargin(m_cell_margin, m_column_params.size(), i));
    }

    // adjust effective minimums based on cell contents
    for (std::map<Wnd*, WndPosition>::iterator it = m_wnd_positions.begin(); it != m_wnd_positions.end(); ++it) {
        Pt min_space_needed = it->first->MinSize();
        if (0 < it->second.first_row && it->second.last_row < static_cast<int>(m_row_params.size()))
            min_space_needed.y += m_cell_margin;
        else if (0 < it->second.first_row || it->second.last_row < static_cast<int>(m_row_params.size()))
            min_space_needed.y += static_cast<int>(std::ceil(m_cell_margin / 2.0));
        if (0 < it->second.first_column && it->second.last_column < static_cast<int>(m_column_params.size()))
            min_space_needed.x += m_cell_margin;
        else if (0 < it->second.first_column || it->second.last_column < static_cast<int>(m_column_params.size()))
            min_space_needed.x += static_cast<int>(std::ceil(m_cell_margin / 2.0));

        // adjust row minimums
        double total_stretch = 0.0;
        for (int i = it->second.first_row; i < it->second.last_row; ++i) {
            total_stretch += m_row_params[i].stretch;
        }
        if (total_stretch) {
            for (int i = it->second.first_row; i < it->second.last_row; ++i) {
                m_row_params[i].effective_min = std::max(m_row_params[i].effective_min, static_cast<int>(min_space_needed.y / total_stretch * m_row_params[i].stretch));
            }
        } else { // if all rows have 0.0 stretch, distribute height evenly
            double per_row_min = min_space_needed.y / static_cast<double>(it->second.last_row - it->second.first_row);
            for (int i = it->second.first_row; i < it->second.last_row; ++i) {
                m_row_params[i].effective_min = std::max(m_row_params[i].effective_min, static_cast<int>(per_row_min + 0.5));
            }
        }

        // adjust column minimums
        total_stretch = 0.0;
        for (int i = it->second.first_column; i < it->second.last_column; ++i) {
            total_stretch += m_column_params[i].stretch;
        }
        if (total_stretch) {
            for (int i = it->second.first_column; i < it->second.last_column; ++i) {
                m_column_params[i].effective_min = std::max(m_column_params[i].effective_min, static_cast<int>(min_space_needed.x / total_stretch * m_column_params[i].stretch));
            }
        } else { // if all columns have 0.0 stretch, distribute width evenly
            double per_column_min = min_space_needed.x / static_cast<double>(it->second.last_column - it->second.first_column);
            for (int i = it->second.first_column; i < it->second.last_column; ++i) {
                m_column_params[i].effective_min = std::max(m_column_params[i].effective_min, static_cast<int>(per_column_min + 0.5));
            }
        }
    }

    // determine final effective minimums, preserving stretch ratios
    double greatest_min_over_stretch_ratio = 0.0;
    for (unsigned int i = 0; i < m_row_params.size(); ++i) {
        if (m_row_params[i].stretch)
            greatest_min_over_stretch_ratio = std::max(greatest_min_over_stretch_ratio, m_row_params[i].effective_min / m_row_params[i].stretch);
    }
    for (unsigned int i = 0; i < m_row_params.size(); ++i) {
        m_row_params[i].effective_min = std::max(m_row_params[i].effective_min, static_cast<int>(m_row_params[i].stretch * greatest_min_over_stretch_ratio));
    }
    greatest_min_over_stretch_ratio = 0.0;
    for (unsigned int i = 0; i < m_column_params.size(); ++i) {
        if (m_column_params[i].stretch)
            greatest_min_over_stretch_ratio = std::max(greatest_min_over_stretch_ratio, m_column_params[i].effective_min / m_column_params[i].stretch);
    }
    for (unsigned int i = 0; i < m_column_params.size(); ++i) {
        m_column_params[i].effective_min = std::max(m_column_params[i].effective_min, static_cast<int>(m_column_params[i].stretch * greatest_min_over_stretch_ratio));
    }

    bool size_or_min_size_changed = false;
    Pt new_min_size(TotalMinWidth(), TotalMinHeight());
    if (new_min_size != MinSize()) {
        SetMinSize(new_min_size);
        size_or_min_size_changed = true;
    }
    Pt original_size = Size();
    Wnd::SizeMove(x1, y1, x2, y2);
    if (Size() != original_size)
        size_or_min_size_changed = true;

    // determine row and column positions
    double total_stretch = TotalStretch(m_row_params);
    int total_stretch_space = Size().y - MinSize().y;
    double space_per_unit_stretch = total_stretch ? total_stretch_space / total_stretch : 0.0;
    bool larger_than_min = 0 < total_stretch_space;
    double remainder = 0.0;
    int current_origin = m_border_margin;
    for (unsigned int i = 0; i < m_row_params.size(); ++i) {
        if (larger_than_min) {
            if (i < m_row_params.size() - 1) {
                double raw_width =
                    m_row_params[i].effective_min +
                    (total_stretch ?
                     space_per_unit_stretch * m_row_params[i].stretch :
                     total_stretch_space / static_cast<double>(m_row_params.size()));
                int int_raw_width = static_cast<int>(raw_width);
                m_row_params[i].current_width = int_raw_width;
                remainder += raw_width - int_raw_width;
                if (1.0 < remainder) {
                    --remainder;
                    ++m_row_params[i].current_width;
                }
            } else {
                m_row_params[i].current_width = (Height() - m_border_margin) - current_origin;
            }
        } else {
            m_row_params[i].current_width = m_row_params[i].effective_min;
        }
        m_row_params[i].current_origin = current_origin;
        current_origin += m_row_params[i].current_width;
    }

    total_stretch = TotalStretch(m_column_params);
    total_stretch_space = Size().x - MinSize().x;
    space_per_unit_stretch = total_stretch ? total_stretch_space / total_stretch : 0.0;
    larger_than_min = 0 < total_stretch_space;
    remainder = 0.0;
    current_origin = m_border_margin;
    for (unsigned int i = 0; i < m_column_params.size(); ++i) {
        if (larger_than_min) {
            if (i < m_column_params.size() - 1) {
                double raw_width =
                    m_column_params[i].effective_min +
                    (total_stretch ?
                     space_per_unit_stretch * m_column_params[i].stretch :
                     total_stretch_space / static_cast<double>(m_column_params.size()));
                int int_raw_width = static_cast<int>(raw_width);
                m_column_params[i].current_width = int_raw_width;
                remainder += raw_width - int_raw_width;
                if (1.0 < remainder) {
                    --remainder;
                    ++m_column_params[i].current_width;
                }
            } else {
                m_column_params[i].current_width = (Width() - m_border_margin) - current_origin;
            }
        } else {
            m_column_params[i].current_width = m_column_params[i].effective_min;
        }
        m_column_params[i].current_origin = current_origin;
        current_origin += m_column_params[i].current_width;
    }

    if (m_row_params.back().current_origin + m_row_params.back().current_width != Height() - m_border_margin)
        throw std::runtime_error("Layout::SizeMove() : calculated row positions do not sum to the height of the layout");

    if (m_column_params.back().current_origin + m_column_params.back().current_width != Width() - m_border_margin)
        throw std::runtime_error("Layout::SizeMove() : calculated column positions do not sum to the width of the layout");

    // resize cells and their contents
    m_ignore_child_resize = true;
    for (std::map<Wnd*, WndPosition>::iterator it = m_wnd_positions.begin(); it != m_wnd_positions.end(); ++it) {
        Pt ul(m_column_params[it->second.first_column].current_origin,
              m_row_params[it->second.first_row].current_origin);
        Pt lr(m_column_params[it->second.last_column - 1].current_origin + m_column_params[it->second.last_column - 1].current_width,
              m_row_params[it->second.last_row - 1].current_origin + m_row_params[it->second.last_row - 1].current_width);
        if (0 < it->second.first_row)
            ul.y += m_cell_margin / 2;
        if (0 < it->second.first_column)
            ul.x += m_cell_margin / 2;
        if (it->second.last_row < static_cast<int>(m_row_params.size()))
            lr.y -= static_cast<int>(m_cell_margin / 2.0 + 0.5);
        if (it->second.last_column < static_cast<int>(m_column_params.size()))
            lr.x -= static_cast<int>(m_cell_margin / 2.0 + 0.5);

        if (it->second.alignment == ALIGN_NONE) { // expand to fill available space
            it->first->SizeMove(ul, lr);
        } else { // align as appropriate
            Pt available_space = lr - ul;
            Pt window_size(std::min(available_space.x, it->second.original_size.x),
                           std::min(available_space.y, it->second.original_size.y));
            Pt resize_ul, resize_lr;
            if (it->second.alignment & ALIGN_LEFT) {
                resize_ul.x = ul.x;
                resize_lr.x = resize_ul.x + window_size.x;
            } else if (it->second.alignment & ALIGN_CENTER) {
                resize_ul.x = ul.x + (available_space.x - window_size.x) / 2;
                resize_lr.x = resize_ul.x + window_size.x;
            } else { // it->second.alignment & ALIGN_RIGHT
                resize_lr.x = lr.x;
                resize_ul.x = resize_lr.x - window_size.x;
            }
            if (it->second.alignment & ALIGN_TOP) {
                resize_ul.y = ul.y;
                resize_lr.y = resize_ul.y + window_size.y;
            } else if (it->second.alignment & ALIGN_VCENTER) {
                resize_ul.y = ul.y + (available_space.y - window_size.y) / 2;
                resize_lr.y = resize_ul.y + window_size.y;
            } else { // it->second.alignment & ALIGN_BOTTOM
                resize_lr.y = lr.y;
                resize_ul.y = resize_lr.y - window_size.y;
            }
            it->first->SizeMove(resize_ul, resize_lr);
        }
    }
    m_ignore_child_resize = false;

    if (ContainingLayout() && size_or_min_size_changed)
        ContainingLayout()->ChildSizeOrMinSizeOrMaxSizeChanged();
}

void Layout::MouseWheel(const Pt& pt, int move, Uint32 keys)
{
    if (Parent())
        Parent()->MouseWheel(pt, move, keys);
}

void Layout::Keypress(Key key, Uint32 key_mods)
{
    if (Parent())
        Parent()->Keypress(key, key_mods);
}

void Layout::Add(Wnd *w, int row, int column, Uint32 alignment/* = 0*/)
{
    Add(w, row, column, row + 1, column + 1, alignment);
}

void Layout::Add(Wnd *w, int first_row, int first_column, int last_row, int last_column, Uint32 alignment/* = 0*/)
{
    assert(0 <= first_row);
    assert(0 <= first_column);
    assert(first_row < last_row);
    assert(first_column < last_column);
    ValidateAlignment(alignment);
    if (m_cells.size() < static_cast<unsigned int>(last_row) || m_cells[0].size() < static_cast<unsigned int>(last_column)) {
        ResizeLayout(std::max(last_row, Rows()), std::max(last_column, Columns()));
    }
    for (int i = first_row; i < last_row; ++i) {
        for (int j = first_column; j < last_column; ++j) {
            if (m_cells[i][j])
                throw std::runtime_error("Layout::Add() : Attempted to add a Wnd to a layout cell that is already occupied");
            m_cells[i][j] = w;
        }
    }
    m_wnd_positions[w] = WndPosition(first_row, first_column, last_row, last_column, alignment, w->Size());
    AttachChild(w);
    RedoLayout();
}

void Layout::ResizeLayout(int rows, int columns)
{
    assert(0 < rows);
    assert(0 < columns);
    if (static_cast<unsigned int>(rows) < m_cells.size()) {
        for (unsigned int i = static_cast<unsigned int>(rows); i < m_cells.size(); ++i) {
            for (unsigned int j = 0; j < m_cells[i].size(); ++j) {
                DeleteChild(m_cells[i][j]);
                m_wnd_positions.erase(m_cells[i][j]);
            }
        }
    }
    m_cells.resize(rows);
    for (unsigned int i = 0; i < m_cells.size(); ++i) {
        if (static_cast<unsigned int>(columns) < m_cells[i].size()) {
            for (unsigned int j = static_cast<unsigned int>(columns); j < m_cells[i].size(); ++j) {
                DeleteChild(m_cells[i][j]);
                m_wnd_positions.erase(m_cells[i][j]);
            }
        }
        m_cells[i].resize(columns);
    }
    m_row_params.resize(rows);
    m_column_params.resize(columns);
    RedoLayout();
}

void Layout::SetChildAlignment(Wnd* wnd, Uint32 alignment)
{
    std::map<Wnd*, WndPosition>::iterator it = m_wnd_positions.find(wnd);
    if (it != m_wnd_positions.end()) {
        ValidateAlignment(alignment);
        it->second.alignment = alignment;
        RedoLayout();
    }
}

void Layout::SetBorderMargin(int margin)
{
    m_border_margin = margin;
    RedoLayout();
}

void Layout::SetCellMargin(int margin)
{
    m_cell_margin = margin;
    RedoLayout();
}

void Layout::SetRowStretch(int row, double stretch)
{
    assert(static_cast<unsigned int>(row) < m_row_params.size());
    m_row_params[row].stretch = stretch;
    RedoLayout();
}

void Layout::SetColumnStretch(int column, double stretch)
{
    assert(static_cast<unsigned int>(column) < m_column_params.size());
    m_column_params[column].stretch = stretch;
    RedoLayout();
}

void Layout::SetMinimumRowHeight(int row, int height)
{
    assert(static_cast<unsigned int>(row) < m_row_params.size());
    m_row_params[row].min = height;
    RedoLayout();
}

void Layout::SetMinimumColumnWidth(int column, int width)
{
    assert(static_cast<unsigned int>(column) < m_column_params.size());
    m_column_params[column].min = width;
    RedoLayout();
}

void Layout::DefineAttributes(WndEditor* editor)
{
    if (!editor)
        return;
    Wnd::DefineAttributes(editor);
    editor->Label("Layout");
    editor->Attribute("Border Margin", m_border_margin);
    editor->Attribute("Cell Margin", m_cell_margin);
    // TODO: handle setting the number of rows and columns
}

double Layout::TotalStretch(const std::vector<RowColParams>& params_vec) const
{
    double retval = 0.0;
    for (unsigned int i = 0; i < params_vec.size(); ++i) {
        retval += params_vec[i].stretch;
    }
    return retval;
}

int Layout::TotalMinWidth() const
{
    int retval = 2 * m_border_margin;
    for (unsigned int i = 0; i < m_column_params.size(); ++i) {
        retval += m_column_params[i].effective_min;
    }
    return retval;
}

int Layout::TotalMinHeight() const
{
    int retval = 2 * m_border_margin;
    for (unsigned int i = 0; i < m_row_params.size(); ++i) {
        retval += m_row_params[i].effective_min;
    }
    return retval;
}

void Layout::ValidateAlignment(Uint32& alignment)
{
    int dup_ct = 0;   // duplication count
    if (alignment & ALIGN_LEFT) ++dup_ct;
    if (alignment & ALIGN_RIGHT) ++dup_ct;
    if (alignment & ALIGN_CENTER) ++dup_ct;
    if (1 < dup_ct) {   // when multiples are picked, use ALIGN_CENTER by default
        alignment &= ~(ALIGN_RIGHT | ALIGN_LEFT);
        alignment |= ALIGN_CENTER;
    }
    dup_ct = 0;
    if (alignment & ALIGN_TOP) ++dup_ct;
    if (alignment & ALIGN_BOTTOM) ++dup_ct;
    if (alignment & ALIGN_VCENTER) ++dup_ct;
    if (1 < dup_ct) {   // when multiples are picked, use ALIGN_VCENTER by default
        alignment &= ~(ALIGN_TOP | ALIGN_BOTTOM);
        alignment |= ALIGN_VCENTER;
    }

    // get rid of any irrelevant bits
    if (!(alignment & (ALIGN_LEFT | ALIGN_RIGHT | ALIGN_CENTER | ALIGN_TOP | ALIGN_BOTTOM | ALIGN_VCENTER)))
        alignment = ALIGN_NONE;
}

void Layout::RedoLayout()
{
    Resize(Size());
}

void Layout::ChildSizeOrMinSizeOrMaxSizeChanged()
{
    if (!m_ignore_child_resize)
        RedoLayout();
}
