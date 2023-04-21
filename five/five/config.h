#pragma once

#include <toml++/toml.h>
#include <string>
#include <cstdint>
#include <iostream> // FIXME
#include <five/type_name.cc>


namespace config {

template <class T>
void saveload(toml::table& tbl, const char* name, const T* t); // save

template <class T>
void saveload(const toml::table& tbl, const char* name, T* t); // load

#define CONFIG_DEFINE_SAVELOAD_FOR_TYPE(type) \
    template <> \
    void saveload(const toml::table& tbl, const char* name, type* t) { /* load */ \
        if (tbl.contains(name)) { \
            *t = *tbl[name].value<type>(); \
        } \
    } \
    template <> \
    void saveload(toml::table& tbl, const char* name, const type* t) { /* save */ \
        tbl.insert_or_assign(name, *t); \
    }

CONFIG_DEFINE_SAVELOAD_FOR_TYPE(std::string);
CONFIG_DEFINE_SAVELOAD_FOR_TYPE(int64_t);

#undef CONFIG_DEFINE_SAVELOAD_FOR_TYPE

}

#define TS_VALUE(name) {::config::saveload(tbl, #name, &name);}

        // template <class T, class D>
        // const auto __config_serialize_op = ::config::load<T, D>;

#define TOML_STRUCT_DEFINE_MEMBER_SAVELOAD(expressions) \
    void config_load(const toml::table& tbl) { \
        expressions \
    } \
    void config_save(toml::table& tbl) const { \
        expressions \
    }

#define TOML_STRUCT_DEFINE_SAVELOAD(self) \
        template <> \
        void ::config::saveload(const toml::table& tbl, const char* name, self* t) { /* load */ \
            const toml::table* inner = tbl[name].as_table(); \
            if (inner) { \
                t->config_load(*inner); \
            } \
        } \
        template <> \
        void ::config::saveload(toml::table& tbl, const char* name, const self* t) { /* save */ \
            auto* inner = tbl.insert_or_assign(name, toml::table()).first->second.as_table(); \
            t->config_save(*inner); \
        }


template <class T>
std::ostream& print(std::ostream& os, const T& conf) {
    toml::table tbl;
    conf.config_save(tbl);
    return os << five::type_name<T>() << "(\n" << tbl << "\n)";
}
