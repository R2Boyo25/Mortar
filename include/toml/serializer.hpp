//              Copyright Toru Niina 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)
#ifndef TOML_SERIALIZER_HPP
#define TOML_SERIALIZER_HPP
#include <toml/value.hpp>
#include <toml/lexer.hpp>
#include <sstream>
#include <locale>
#include <iomanip>

namespace toml
{
namespace detail
{
inline bool is_array_of_table(const value& v)
{
    return v.is(value::array_tag) && (!v.get<array>().empty()) &&
           v.get<array>().front().is(value::table_tag);
}

struct serializer : boost::static_visitor<std::string>
{
    serializer(const bool can_be_inlinized, const std::size_t w)
        : can_be_inlinized_(can_be_inlinized), width_(w)
    {}
    serializer(const bool can_be_inlinized, const std::size_t w,
               const std::vector<toml::key>& ks)
        : can_be_inlinized_(can_be_inlinized), width_(w), keys_(ks)
    {}
    ~serializer(){}

    std::string operator()(const boost::blank) const
    {
        return "<blank>";
    }
    std::string operator()(const boolean b) const
    {
        return b ? "true" : "false";
    }
    std::string operator()(const integer i) const
    {
        std::ostringstream oss; oss << i;
        return oss.str();
    }
    std::string operator()(const floating f) const
    {
        std::ostringstream oss; oss << std::showpoint << f;
        std::string token(oss.str());
        {
            std::string::iterator E = std::find(token.begin(), token.end(), 'E');
            if(E != token.end()){*E = 'e';}
        }
        std::string::iterator e = std::find(token.begin(), token.end(), 'e');
        if(e == token.end()){return token;}
        // zero-prefix in an exponent part is not allowed in TOML.
        std::string exp;
        for(std::reverse_iterator<std::string::iterator>
                iter(token.end()), iend(token.begin()); iter != iend; ++iter)
        {
            if('0' <= *iter && *iter <= '9'){exp += *iter;}
            else{break;}
        }
        token.erase(token.size() - exp.size());
        while(exp.size() > 1 && exp.at(exp.size()-1) == '0')
        {exp.erase(exp.size()-1, 1);}
        std::reverse_copy(exp.begin(), exp.end(), std::back_inserter(token));
        return token;
    }
    std::string operator()(const string& s) const
    {
        if(s.kind == string::basic)
        {
            if(std::find(s.str.begin(), s.str.end(), '\n') != s.str.end())
            {
                const std::string open("\"\"\"\n");
                const std::string close("\\\n\"\"\"");
                const std::string b = escape_ml_basic_string(s.str);
                return open + b + close;
            }

            std::string oneline = escape_basic_string(s.str);
            if(oneline.size() + 2 < width_ || width_ < 2)
            {
                const std::string quote("\"");
                return quote + oneline + quote;
            }
            // split into multiple lines...
            std::string token("\"\"\"\n");
            while(!oneline.empty())
            {
                if(oneline.size() < width_)
                {
                    token += oneline;
                    oneline.clear();
                }
                else if(oneline.at(width_ - 2) == '\\')
                {
                    token += oneline.substr(0, width_-2);
                    token += "\\\n";
                    oneline.erase(0, width_-2);
                }
                else
                {
                    token += oneline.substr(0, width_-1);
                    token += "\\\n";
                    oneline.erase(0, width_-1);
                }
            }
            return token + std::string("\\\n\"\"\"");
        }
        else
        {
            if(std::find(s.str.begin(), s.str.end(), '\n') != s.str.end() ||
               std::find(s.str.begin(), s.str.end(), '\'') != s.str.end() )
            {
                const std::string open("'''\n");
                const std::string close("'''");
                return open + s.str + close;
            }
            else
            {
                const std::string quote("'");
                return quote + s.str + quote;
            }
        }
    }

    std::string operator()(const date& v) const
    {
        std::ostringstream oss;
        boost::gregorian::date_facet*
            facet(new boost::gregorian::date_facet("%Y-%m-%d"));
        oss.imbue(std::locale(oss.getloc(), facet));
        oss << v;
        return oss.str();
    }
    std::string operator()(const time& v) const
    {
        std::ostringstream oss;
        boost::gregorian::date_facet*
            facet(new boost::gregorian::date_facet("%F *"));
        oss.imbue(std::locale(oss.getloc(), facet));
        oss << v;
        return oss.str();
    }
    std::string operator()(const local_datetime& v) const
    {
        std::ostringstream oss;
        boost::posix_time::time_facet*
            facet(new boost::posix_time::time_facet("%Y-%m-%dT%H:%M:%S%F"));
        oss.imbue(std::locale(oss.getloc(), facet));
        oss << v;
        return oss.str();
    }
    std::string operator()(const offset_datetime& v) const
    {
        std::ostringstream oss;
        boost::local_time::local_time_facet*
            facet(new boost::local_time::local_time_facet(
                        "%Y-%m-%dT%H:%M:%S%F%Q"));
        oss.imbue(std::locale(oss.getloc(), facet));
        oss << v;
        return oss.str();
    }
    std::string operator()(const array& v) const
    {
        if(!v.empty() && v.front().is(value::table_tag)) // array of tables
        {
            std::string token;
            if(this->can_be_inlinized_)
            {
                if(!keys_.empty())
                {
                    token += serialize_key(keys_.back());
                    token += " = ";
                }
                bool width_exceeds = false;
                token += "[\n";
                for(array::const_iterator i(v.begin()), e(v.end()); i!=e; ++i)
                {
                    const std::string t = make_inline_table(i->get<table>());
                    if(t.size()+1 > width_ || // +1 for `,`
                       std::find(t.begin(), t.end(), '\n') != t.end())
                    {
                        width_exceeds = true;
                        break;
                    }
                    token += t;
                    token += ",\n";
                }
                if(!width_exceeds)
                {
                    token += "]\n";
                    return token;
                }
            }

            token.clear();
            for(array::const_iterator i(v.begin()), e(v.end()); i!=e; ++i)
            {
                token += "[[";
                token += serialize_dotted_key(keys_);
                token += "]]\n";
                token += make_multiline_table(i->get<table>());
            }
            return token;
        }

        // not an array of tables.
        {
            const std::string inl = this->make_inline_array(v);
            if(inl.size() < this->width_ &&
               std::find(inl.begin(), inl.end(), '\n') == inl.end())
            {
                return inl;
            }
        }

        // if the length exceeds this->width_, print multiline array
        std::string token;
        token += "[\n";
        for(array::const_iterator i(v.begin()), e(v.end()); i!=e; ++i)
        {
            token += apply_visitor(*this, *i);
            token += ",\n";
        }
        token += "]\n";
        return token;
    }

    std::string operator()(const table& v) const
    {
        if(this->can_be_inlinized_)
        {
            std::string token;
            if(!this->keys_.empty())
            {
                token += this->serialize_key(this->keys_.back());
                token += " = ";
            }
            token += this->make_inline_table(v);
            if(token.size() < this->width_)
            {
                return token;
            }
        }

        std::string token;
        if(!keys_.empty())
        {
            token += '[';
            token += serialize_dotted_key(keys_);
            token += "]\n";
        }
        token += make_multiline_table(v);
        return token;
    }

  private:

    std::string escape_basic_string(const std::string& s) const
    {
        std::string retval;
        for(std::string::const_iterator i(s.begin()), e(s.end()); i!=e; ++i)
        {
            switch(*i)
            {
                case '\\': retval += "\\\\"; break;
                case '\"': retval += "\\\""; break;
                case '\b': retval += "\\b";  break;
                case '\t': retval += "\\t";  break;
                case '\f': retval += "\\f";  break;
                case '\n': retval += "\\n";  break;
                case '\r': retval += "\\r";  break;
                default  : retval += *i;     break;
            }
        }
        return retval;
    }

    std::string escape_ml_basic_string(const std::string& s) const
    {
        std::string retval;
        for(std::string::const_iterator i(s.begin()), e(s.end()); i!=e; ++i)
        {
            switch(*i)
            {
                case '\\': retval += "\\\\"; break;
                case '\"': retval += "\\\""; break;
                case '\b': retval += "\\b";  break;
                case '\t': retval += "\\t";  break;
                case '\f': retval += "\\f";  break;
                case '\n': retval += "\n";   break;
                case '\r':
                {
                    if((i+1) != e && *(i+1) == '\n') {retval += "\r\n"; ++i;}
                    else                             {retval += "\\r";}
                    break;
                }
                default  : retval += *i;     break;
            }
        }
        return retval;
    }

    std::string serialize_key(const toml::key& key) const
    {
        toml::key::const_iterator i(key.begin());
        lex_unquoted_key::invoke(i, key.end());
        if(i == key.end())
        {
            return key;
        }
        std::string token("\"");
        token += escape_basic_string(key);
        token += "\"";
        return token;
    }

    std::string serialize_dotted_key(const std::vector<toml::key>& keys) const
    {
        std::string token;
        if(keys.empty()){return token;}

        for(std::vector<toml::key>::const_iterator
                i(keys.begin()), e(keys.end()); i!=e; ++i)
        {
            token += this->serialize_key(*i);
            token += '.';
        }
        token.erase(token.size() - 1, 1); // remove trailing `.`
        return token;
    }

    std::string make_inline_array(const array& v) const
    {
        std::string token;
        token += '[';
        for(array::const_iterator i(v.begin()), e(v.end()); i!=e; ++i)
        {
            if(i != v.begin()) {token += ',';}
            token += apply_visitor(serializer(true,
                        std::numeric_limits<std::size_t>::max()), *i);
        }
        token += ']';
        return token;
    }

    std::string make_inline_table(const table& v) const
    {
        assert(this->can_be_inlinized_);
        std::string token;
        token += '{';
        for(table::const_iterator i(v.begin()), e(v.end()); i!=e; ++i)
        {
            if(i != v.begin()) {token += ',';}
            token += i->first;
            token += '=';
            token += apply_visitor(serializer(true,
                        std::numeric_limits<std::size_t>::max()), i->second);
        }
        token += '}';
        return token;
    }

    std::string make_multiline_table(const table& v) const
    {
        std::string token;
        // print non-table stuff first
        for(table::const_iterator i(v.begin()), e(v.end()); i!=e; ++i)
        {
            if(i->second.is(value::table_tag) || is_array_of_table(i->second))
            {continue;}

            token += serialize_key(i->first);
            token += " = ";
            token += apply_visitor(serializer(true, width_), i->second);
            token += "\n";
        }

        // normal tables / array of tables
        bool multiline_table_printed = false;
        for(table::const_iterator i(v.begin()), e(v.end()); i!=e; ++i)
        {
            if(!i->second.is(value::table_tag) && !is_array_of_table(i->second))
            {
                continue;
            }

            std::vector<toml::key> ks(this->keys_);
            ks.push_back(i->first);

            std::string tmp = apply_visitor(
                serializer(!multiline_table_printed, width_, ks), i->second);
            if((!multiline_table_printed) &&
               std::find(tmp.begin(), tmp.end(), '\n') != tmp.end())
            {
                multiline_table_printed = true;
            }
            else
            {
                // still inline tables. need to add newline after this
                tmp += '\n';
            }
            token += tmp;
        }
        return token;
    }

  private:

    bool        can_be_inlinized_;
    std::size_t width_;
    std::vector<toml::key> keys_;
};
} // detail

inline std::string format(const value& v)
{
    return v.apply_visitor(detail::serializer(false, 80));
}
inline std::string format(const table& t)
{
    return detail::serializer(false, 80)(t);
}

inline std::string format(const value& v, std::size_t w)
{
    return v.apply_visitor(detail::serializer(true, w));
}
inline std::string format(const table& t, std::size_t w)
{
    return detail::serializer(true, w)(t);
}

template<typename charT, typename traits>
std::basic_ostream<charT, traits>&
operator<<(std::basic_ostream<charT, traits>& os, const value& v)
{
    const std::size_t w = os.width();
    os << v.apply_visitor(detail::serializer(false, (w > 2 ? w : 80)));
    return os;
}

} // toml
#endif// TOML_SERIALIZER_HPP
