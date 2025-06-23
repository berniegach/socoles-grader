#include "admin.h"
#include <map>
#include "utils.h"

using std::map;
using pl = Grader::property_level;
using po = Grader::property_order;

Admin::Admin(pl syntax_level, pl semantics_level, pl results_level, po propert_order, bool check_order)
{
    // initialize the grading parameters
    this->syntax_level = syntax_level;
    this->semantics_level = semantics_level;
    this->results_level = results_level;
    this->property_order = propert_order;
    this->check_order = check_order;
    syntax_minor_incorrect_ted = 4;
    semantics_minor_incorrect_ted = 4;
}

void Admin::init(pl syntax_level, pl semantics_level, pl results_level, po propert_order, bool check_order)
{
    // initialize the grading parameters
    this->syntax_level = syntax_level;
    this->semantics_level = semantics_level;
    this->results_level = results_level;
    this->property_order = propert_order;
    this->check_order = check_order;
}

void Admin::init(pl syntax_level, pl semantics_level, pl results_level, po propert_order, bool check_order, int text_edit_distance, int tree_edit_distance)
{
    // initialize the grading parameters
    this->syntax_level = syntax_level;
    this->semantics_level = semantics_level;
    this->results_level = results_level;
    this->property_order = propert_order;
    this->check_order = check_order;
    syntax_minor_incorrect_ted = text_edit_distance;
    semantics_minor_incorrect_ted = tree_edit_distance;
}

string Admin::get_connection_string() const
{
    return connection_string;
}

pl Admin::get_syntax_sensitivity() const
{
    return syntax_level;
}

pl Admin::get_semantics_sensitivity() const
{
    return semantics_level;
}

pl Admin::get_results_sensitivity() const
{
    return results_level;
}

po Admin::get_property_order() const
{
    return property_order;
}

bool Admin::get_check_order() const
{
    return check_order;
}

int Admin::get_syntax_minor_incorrect_ted() const
{
    return syntax_minor_incorrect_ted;
}

int Admin::get_semantics_minor_incorrect_ted() const
{
    return semantics_minor_incorrect_ted;
}