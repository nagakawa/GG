/*
    Copyright 2005-2007 Adobe Systems Incorporated
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html)
*/

/****************************************************************************************************/

#ifndef ADOBE_EDIT_TEXT_HPP
#define ADOBE_EDIT_TEXT_HPP

/****************************************************************************************************/

#include <GG/adobe/config.hpp>

#include <GG/adobe/layout_attributes.hpp>
#include <GG/adobe/any_regular.hpp>
#include <GG/adobe/future/widgets/headers/widget_utils.hpp>

#include <GG/adobe/future/widgets/headers/platform_label.hpp>
#include <GG/adobe/future/widgets/headers/edit_text_common.hpp>

#include <boost/function.hpp>

#include <string>


namespace GG {
    class Edit;
}

/****************************************************************************************************/

namespace adobe {

/****************************************************************************************************/

namespace implementation {

/****************************************************************************************************/

class EditTextFilter;

/****************************************************************************************************/

}

/****************************************************************************************************/

struct edit_text_t : boost::noncopyable
{
    typedef std::string                                         model_type;
    typedef boost::function<void (const std::string&, bool&)>   edit_text_pre_edit_proc_t;
    typedef boost::function<void (const model_type&)>           setter_type;

                      edit_text_t(const edit_text_ctor_block_t& block);

    label_t&          get_label();

// placeable

    void              measure(extents_t& result);
    void              place(const place_data_t& place_data);

//controller
    void              monitor(setter_type proc);
    void              enable(bool active);

//view
    void              display(const model_type&);

    void              set_field_text(const std::string& text);
    void              set_static_disabled(bool is_static_disabled);
    void              signal_pre_edit(edit_text_pre_edit_proc_t proc);

    GG::Edit*                  control_m;
    unsigned int               original_height_m;
    GG::Clr                    color_m;
    GG::Clr                    text_color_m;
    GG::Clr                    interior_color_m;
    label_t                    name_m;
    std::string                alt_text_m;
    std::string                field_text_m;
    bool                       using_label_m;
    long                       characters_m;
    long                       rows_m;
    long                       cols_m;
    long                       max_cols_m;
    bool                       read_only_m;
    bool                       terminal_style_m;
    bool                       wrap_m;
    bool                       scrollable_m;
    bool                       password_m;
    name_t                     signal_id_m;
    edit_text_pre_edit_proc_t  pre_edit_proc_m;
    setter_type                post_edit_proc_m;
    std::string                value_m; // Used to debounce

    boost::shared_ptr<implementation::EditTextFilter>
                               filter_m;

    boost::function<void (const std::string&)> edited_proc_m;
    boost::function<void (const std::string&)> focus_update_proc_m;

    implementation::color_proxy_t color_proxy_m;
    implementation::color_proxy_t text_color_proxy_m;
    implementation::color_proxy_t interior_color_proxy_m;
};

const std::string& get_control_string(const edit_text_t& widget);

inline GG::Edit* get_display(edit_text_t& widget)
{ return widget.control_m; }

/****************************************************************************************************/

} // namespace adobe

/****************************************************************************************************/

#endif

/****************************************************************************************************/
