module;
#include <rfl/json.hpp>
export module erdo:json;
import std;

namespace erdo::json
{
    std::unordered_map<std::uintptr_t, std::shared_ptr<const void>> ids;

    void reset_ids()
    {
        ids.clear();
    }

    template<typename T>
    struct shared_ptr_impl
    {
        std::uintptr_t id{};
        T* value{};

        static shared_ptr_impl from_class(const std::shared_ptr<T>& p) noexcept
        {
            if (!p)
                return {};

            auto [it, inserted] = ids.emplace(reinterpret_cast<std::uintptr_t>(p.get()),p);

            return {
                it->first,
                inserted ? p.get() : nullptr
            };
        }

        std::shared_ptr<T> to_class()
        {
            if (id == 0)
                return {};

            if (value)
            {
                auto res = std::shared_ptr<T>(value);
                value = nullptr;

                auto [it, inserted] =
                    ids.emplace(id, res);

                if (!inserted)
                    throw std::runtime_error("Duplicate id found in shared_ptr_impl::to_class");

                return res;
            }

            return std::static_pointer_cast<T>(std::const_pointer_cast<void>(ids.at(id)));
        }
    };
}

namespace rfl::parsing
{
    template<typename T, typename ReaderType, typename WriterType, typename ProcessorsType>
    struct Parser<ReaderType, WriterType, std::shared_ptr<T>, ProcessorsType>
        : public CustomParser<ReaderType, WriterType, ProcessorsType, std::shared_ptr<T>, erdo::json::shared_ptr_impl<T>> {};
}

export namespace erdo::json
{
    template <typename...Ps>
    auto write(const auto& _obj, const yyjson_write_flag _flag = 0)
    {
        reset_ids();
        return rfl::json::write<Ps...>(_obj, _flag);
    }

    template <typename T, typename...Ps>
    auto read(const auto& _obj)
    {
        reset_ids();
        return rfl::json::read<T, rfl::AllowRawPtrs, Ps...>(_obj).value();
    }

    template <typename...Ps>
    auto save(const std::filesystem::path& file, const auto& _obj, const yyjson_write_flag _flag = 0)
    {
        reset_ids();
        return rfl::json::save<Ps...>(file.string(), _obj, _flag).value();
    }

    template <typename T, typename...Ps>
    auto load(const std::filesystem::path& file)
    {
        reset_ids();
        return rfl::json::load<T, rfl::AllowRawPtrs, Ps...>(file.string()).value();
    }
}