 /*
    Copyright 2005-2007 Adobe Systems Incorporated
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html)
*/

/****************************************************************************************************/

#ifndef ADOBE_EDIT_NUMBER_FACTORY_HPP
#define ADOBE_EDIT_NUMBER_FACTORY_HPP

#include <GG/adobe/dictionary.hpp>

/****************************************************************************************************/

namespace adobe {

/****************************************************************************************************/

struct widget_node_t;
struct factory_token_t;
class widget_factory_t;

/****************************************************************************************************/

widget_node_t make_edit_number(const dictionary_t&     parameters, 
                               const widget_node_t&    parent, 
                               const factory_token_t&  token,
                               const widget_factory_t& factory);

/****************************************************************************************************/

} // namespace adobe

/****************************************************************************************************/

#endif

/****************************************************************************************************/
