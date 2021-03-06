#if USE_SDL_BACKEND
#include "SDLBackend.h"
#else
#include "OgreBackend.h"
#endif

#include <GG/EveGlue.h>
#include <GG/EveParser.h>
#include <GG/Filesystem.h>
#include <GG/GUI.h>
#include <GG/Timer.h>
#include <GG/Wnd.h>
#include <GG/adobe/adam.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <fstream>

#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include "TestingUtils.h"


adobe::any_regular_t increment(const adobe::dictionary_t& parameters)
{
    double retval;
    get_value(parameters, adobe::static_name_t("value"), retval);
    ++retval;
    return adobe::any_regular_t(retval);
}

adobe::any_regular_t push_back(const adobe::dictionary_t& parameters)
{
    adobe::any_regular_t state;
    get_value(parameters, adobe::static_name_t("state"), state);
    adobe::array_t& array = state.cast<adobe::array_t>();
    adobe::any_regular_t value;
    get_value(parameters, adobe::static_name_t("value"), value);
    array.push_back(value);
    return state;
}

adobe::any_regular_t push_back_key(const adobe::dictionary_t& parameters)
{
    adobe::any_regular_t state;
    get_value(parameters, adobe::static_name_t("state"), state);
    adobe::array_t& array = state.cast<adobe::array_t>();
    adobe::any_regular_t key;
    get_value(parameters, adobe::static_name_t("key"), key);
    array.push_back(key);
    return state;
}

struct Test
{
    Test() {}
    Test(const char* expression,
         const adobe::any_regular_t& expected_result,
         bool expect_exception = false) :
        m_expression(expression),
        m_expected_result(expected_result),
        m_expect_exception(expect_exception)
        {}
    const char* m_expression;
    adobe::any_regular_t m_expected_result;
    bool m_expect_exception;
};

const std::vector<Test>& Tests()
{
    static std::vector<Test> retval;
    if (retval.empty()) {
        {
            adobe::dictionary_t result;
            result[adobe::static_name_t("one")] = adobe::any_regular_t(2.0);
            result[adobe::static_name_t("two")] = adobe::any_regular_t(3.0);
            retval.push_back(Test("transform({one: 1, two: 2}, @increment)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(4.0));
            result.push_back(adobe::any_regular_t(5.0));
            retval.push_back(Test("transform([3, 4], @increment)", adobe::any_regular_t(result)));
        }
        retval.push_back(Test("transform(5, @increment)", adobe::any_regular_t(6.0)));

        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(adobe::static_name_t("one")));
            result.push_back(adobe::any_regular_t(adobe::static_name_t("two")));
            result.push_back(adobe::any_regular_t(adobe::static_name_t("three")));
            retval.push_back(Test("fold({one: 1, two: 2, three: 3}, [], @push_back_key)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(4.0));
            result.push_back(adobe::any_regular_t(5.0));
            result.push_back(adobe::any_regular_t(6.0));
            retval.push_back(Test("fold([4, 5, 6], [], @push_back)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(7.0));
            retval.push_back(Test("fold(7, [], @push_back)", adobe::any_regular_t(result)));
        }

        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(3.0));
            result.push_back(adobe::any_regular_t(2.0));
            result.push_back(adobe::any_regular_t(1.0));
            retval.push_back(Test("foldr({one: 1, two: 2, three: 3}, [], @push_back)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(6.0));
            result.push_back(adobe::any_regular_t(5.0));
            result.push_back(adobe::any_regular_t(4.0));
            retval.push_back(Test("foldr([4, 5, 6], [], @push_back)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(7.0));
            retval.push_back(Test("foldr(7, [], @push_back)", adobe::any_regular_t(result)));
        }

        retval.push_back(Test("append()", adobe::any_regular_t(), true));
        {
            adobe::array_t result;
            retval.push_back(Test("append([])", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(7.0));
            retval.push_back(Test("append([], 7)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(7.0));
            result.push_back(adobe::any_regular_t(std::string("8")));
            retval.push_back(Test("append([7], '8')", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(7.0));
            result.push_back(adobe::any_regular_t(std::string("8")));
            result.push_back(adobe::any_regular_t(adobe::name_t("nine")));
            retval.push_back(Test("append([7], '8', @nine)", adobe::any_regular_t(result)));
        }

        retval.push_back(Test("prepend()", adobe::any_regular_t(), true));
        {
            adobe::array_t result;
            retval.push_back(Test("prepend([])", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(7.0));
            retval.push_back(Test("prepend([], 7)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(7.0));
            result.push_back(adobe::any_regular_t(std::string("8")));
            retval.push_back(Test("prepend(['8'], 7)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(7.0));
            result.push_back(adobe::any_regular_t(std::string("8")));
            result.push_back(adobe::any_regular_t(adobe::name_t("nine")));
            retval.push_back(Test("prepend([@nine], 7, '8')", adobe::any_regular_t(result)));
        }

        retval.push_back(Test("insert()", adobe::any_regular_t(), true));

        // insert() on arrays
        retval.push_back(Test("insert([])", adobe::any_regular_t(), true));
        retval.push_back(Test("insert([], 'foo')", adobe::any_regular_t(), true));
        retval.push_back(Test("insert([], 1)", adobe::any_regular_t(), true));
        retval.push_back(Test("insert([], -1)", adobe::any_regular_t(), true));
        {
            adobe::array_t result;
            retval.push_back(Test("insert([], 0)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(std::string("one")));
            retval.push_back(Test("insert([], 0, 'one')", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(std::string("one")));
            result.push_back(adobe::any_regular_t(2));
            retval.push_back(Test("insert([], 0, 'one', 2)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(std::string("one")));
            retval.push_back(Test("insert(['one'], 0)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(std::string("one")));
            result.push_back(adobe::any_regular_t(2));
            retval.push_back(Test("insert([2], 0, 'one')", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(std::string("one")));
            result.push_back(adobe::any_regular_t(2));
            result.push_back(adobe::any_regular_t(adobe::name_t("three")));
            retval.push_back(Test("insert([@three], 0, 'one', 2)", adobe::any_regular_t(result)));
        }

        // insert() on dictionaries
        retval.push_back(Test("insert({})", adobe::any_regular_t(), true));
        retval.push_back(Test("insert({}, @foo)", adobe::any_regular_t(), true));
        retval.push_back(Test("insert({}, [1])", adobe::any_regular_t(), true));
        {
            adobe::dictionary_t result;
            result[adobe::static_name_t("foo")] = adobe::any_regular_t(std::string("bar"));
            retval.push_back(Test("insert({}, @foo, 'bar')", adobe::any_regular_t(result)));
        }
        {
            adobe::dictionary_t result;
            result[adobe::static_name_t("foo")] = adobe::any_regular_t(std::string("baz"));
            retval.push_back(Test("insert({foo: 'bar'}, @foo, 'baz')", adobe::any_regular_t(result)));
        }
        {
            adobe::dictionary_t result;
            result[adobe::static_name_t("foo")] = adobe::any_regular_t(std::string("bar"));
            retval.push_back(Test("insert({foo: 'bar'}, {})", adobe::any_regular_t(result)));
        }
        {
            adobe::dictionary_t result;
            result[adobe::static_name_t("foo")] = adobe::any_regular_t(std::string("bar"));
            result[adobe::static_name_t("baz")] = adobe::any_regular_t(1);
            retval.push_back(Test("insert({foo: 'bar'}, {baz: 1})", adobe::any_regular_t(result)));
        }

        retval.push_back(Test("erase()", adobe::any_regular_t(), true));

        // erase() on arrays
        retval.push_back(Test("erase([])", adobe::any_regular_t(), true));
        retval.push_back(Test("erase([], 'foo')", adobe::any_regular_t(), true));
        retval.push_back(Test("erase([], 1)", adobe::any_regular_t(), true));
        retval.push_back(Test("erase([], 0)", adobe::any_regular_t(), true));
        retval.push_back(Test("erase([], -1)", adobe::any_regular_t(), true));
        {
            adobe::array_t result;
            retval.push_back(Test("erase([@foo], 0)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(2));
            result.push_back(adobe::any_regular_t(adobe::name_t("three")));
            retval.push_back(Test("erase(['one', 2, @three], 0)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(std::string("one")));
            result.push_back(adobe::any_regular_t(adobe::name_t("three")));
            retval.push_back(Test("erase(['one', 2, @three], 1)", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(std::string("one")));
            result.push_back(adobe::any_regular_t(2));
            retval.push_back(Test("erase(['one', 2, @three], 2)", adobe::any_regular_t(result)));
        }

        // erase() on dictionaries
        retval.push_back(Test("erase({})", adobe::any_regular_t(), true));
        retval.push_back(Test("erase({}, [1])", adobe::any_regular_t(), true));
        retval.push_back(Test("erase({foo: 'bar'}, @foo, 1)", adobe::any_regular_t(), true));
        {
            adobe::dictionary_t result;
            retval.push_back(Test("erase({one: 2, two: 3}, @one, @two)", adobe::any_regular_t(result)));
        }

        retval.push_back(Test("parse()", adobe::any_regular_t(), true));
        retval.push_back(Test("parse(@one)", adobe::any_regular_t(), true));
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(adobe::name_t("one")));
            result.push_back(adobe::any_regular_t(adobe::name_t(".name_k")));
            result.push_back(adobe::any_regular_t(2));
            result.push_back(adobe::any_regular_t(2));
            result.push_back(adobe::any_regular_t(adobe::name_t(".array")));
            retval.push_back(Test("parse('[@one, 2]')", adobe::any_regular_t(result)));
        }
        retval.push_back(Test("parse('')", adobe::any_regular_t()));
        retval.push_back(Test("parse('[')", adobe::any_regular_t()));

        retval.push_back(Test("eval()", adobe::any_regular_t(), true));
        retval.push_back(Test("eval(@one)", adobe::any_regular_t(), true));
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(adobe::name_t("one")));
            result.push_back(adobe::any_regular_t(2));
            retval.push_back(Test("eval(parse('[@one, 2]'))", adobe::any_regular_t(result)));
        }

        retval.push_back(Test("size()", adobe::any_regular_t(), true));
        retval.push_back(Test("size(1)", adobe::any_regular_t(), true));
        retval.push_back(Test("size(@one)", adobe::any_regular_t(), true));
        retval.push_back(Test("size([])", adobe::any_regular_t(0)));
        retval.push_back(Test("size([1, @two])", adobe::any_regular_t(2)));
        retval.push_back(Test("size({})", adobe::any_regular_t(0)));
        retval.push_back(Test("size({one: 1, two: @two, three: '3'})", adobe::any_regular_t(3)));

        retval.push_back(Test("split()", adobe::any_regular_t(), true));
        retval.push_back(Test("split('a', 'b', 'c')", adobe::any_regular_t(), true));
        retval.push_back(Test("split(1, 'a')", adobe::any_regular_t(), true));
        retval.push_back(Test("split('a', 1)", adobe::any_regular_t(), true));
        retval.push_back(Test("split('a', '')", adobe::any_regular_t(), true));
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(std::string("")));
            retval.push_back(Test("split('', '\n')", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(std::string("foo")));
            result.push_back(adobe::any_regular_t(std::string("bar")));
            retval.push_back(Test("split('foo\nbar', '\n')", adobe::any_regular_t(result)));
        }
        {
            adobe::array_t result;
            result.push_back(adobe::any_regular_t(std::string("foo")));
            result.push_back(adobe::any_regular_t(std::string("")));
            result.push_back(adobe::any_regular_t(std::string("bar")));
            result.push_back(adobe::any_regular_t(std::string("")));
            retval.push_back(Test("split('foo\n\nbar\n', '\n')", adobe::any_regular_t(result)));
        }

        retval.push_back(Test("join()", adobe::any_regular_t(), true));
        retval.push_back(Test("join(1)", adobe::any_regular_t(), true));
        retval.push_back(Test("join(['a'], 1)", adobe::any_regular_t(), true));
        retval.push_back(Test("join(['a', 1])", adobe::any_regular_t(), true));
        retval.push_back(Test("join(['a', 'b'])", adobe::any_regular_t(std::string("ab"))));
        retval.push_back(Test("join(['a', 'b'], ' ')", adobe::any_regular_t(std::string("a b"))));
        retval.push_back(Test("join(split('foo\n\nbar\n', '\n'), '\n')", adobe::any_regular_t(std::string("foo\n\nbar\n"))));

        retval.push_back(Test("assert()", adobe::any_regular_t(), true));
        retval.push_back(Test("assert(true)", adobe::any_regular_t(), true));
        retval.push_back(Test("assert(true, false)", adobe::any_regular_t(), true));
        retval.push_back(Test("assert(true, 'message')", adobe::any_regular_t()));
        retval.push_back(Test("assert(false, 'message')", adobe::any_regular_t(), true));
    }
    return retval;
}

void CheckResult(const GG::EveDialog& eve_dialog, const adobe::any_regular_t& expected_result)
{
    GG::GUI::GetGUI()->HandleGGEvent(GG::GUI::LPRESS, GG::Key(), boost::uint32_t(), GG::Flags<GG::ModKey>(), GG::Pt(GG::X(48), GG::Y(22)), GG::Pt());
    GG::GUI::GetGUI()->HandleGGEvent(GG::GUI::LRELEASE, GG::Key(), boost::uint32_t(), GG::Flags<GG::ModKey>(), GG::Pt(GG::X(48), GG::Y(22)), GG::Pt());
    BOOST_CHECK(eve_dialog.TerminatingAction());
    BOOST_CHECK(eve_dialog.Result().find(adobe::static_name_t("value")) != eve_dialog.Result().end());
    BOOST_CHECK(eve_dialog.Result().find(adobe::static_name_t("value"))->second == expected_result);
}

bool ButtonHandler(adobe::name_t name, const adobe::any_regular_t&)
{ return false; }

void RunTest(std::size_t i)
{
    const Test& test = Tests()[i];
    boost::filesystem::path eve("function_test_dialog.eve");
    std::string adam_str =
        "sheet function_test_dialog { output: result <== { value: ";
    adam_str += test.m_expression;
    adam_str += " }; }";
    boost::filesystem::ifstream eve_stream(eve);
    std::istringstream adam_stream(adam_str);
    if (test.m_expect_exception) {
        BOOST_CHECK_THROW(
            (GG::MakeEveDialog(eve_stream,
                               "function_test_dialog.eve",
                               adam_stream,
                               "inline Adam expression",
                               &ButtonHandler)),
            adobe::stream_error_t
        );
        return;
    }
    GG::EveDialog* eve_dialog =
        GG::MakeEveDialog(eve_stream,
                          "function_test_dialog.eve",
                          adam_stream,
                          "inline Adam expression",
                          &ButtonHandler);
    GG::Timer timer(25);
    GG::Connect(timer.FiredSignal,
                boost::bind(&CheckResult,
                            boost::cref(*eve_dialog),
                            boost::cref(test.m_expected_result)));
    eve_dialog->Run();
}

void CustomInit()
{
    for (std::size_t i = 0; i < Tests().size(); ++i) {
        RunTest(i);
    }

    GG::GUI::GetGUI()->Exit(0);
}

BOOST_AUTO_TEST_CASE( eve_layout )
{
#if USE_SDL_BACKEND
    MinimalSDLGUI::CustomInit = &CustomInit;
    MinimalSDLMain();
#else
    MinimalOgreGUI::CustomInit = &CustomInit;
    MinimalOgreMain();
#endif
}

// Most of this is boilerplate cut-and-pasted from Boost.Test.  We need to
// select which test(s) to do, so we can't use it here unmodified.

#ifdef BOOST_TEST_ALTERNATIVE_INIT_API
bool init_unit_test()                   {
#else
::boost::unit_test::test_suite*
init_unit_test_suite( int, char* [] )   {
#endif

#ifdef BOOST_TEST_MODULE
    using namespace ::boost::unit_test;
    assign_op( framework::master_test_suite().p_name.value, BOOST_TEST_STRINGIZE( BOOST_TEST_MODULE ).trim( "\"" ), 0 );
    
#endif

#ifdef BOOST_TEST_ALTERNATIVE_INIT_API
    return true;
}
#else
    return 0;
}
#endif

int BOOST_TEST_CALL_DECL
main( int argc, char* argv[] )
{
    GG::RegisterDictionaryFunction(adobe::static_name_t("increment"), &increment);
    GG::RegisterDictionaryFunction(adobe::static_name_t("push_back"), &push_back);
    GG::RegisterDictionaryFunction(adobe::static_name_t("push_back_key"), &push_back_key);
    return ::boost::unit_test::unit_test_main( &init_unit_test, argc, argv );
}
