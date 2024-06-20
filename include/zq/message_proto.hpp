#pragma once

#include <google/protobuf/message.h>
#include "message.hpp"

namespace zq {

  template <typename T>
  concept protobuf_message = std::is_base_of_v<google::protobuf::Message, T>;

  template <typename T>
  inline TypedMessage typed_message(const T& value)
    requires std::is_base_of_v<google::protobuf::Message, T>
  {
    Message payload{value.ByteSizeLong()};
    const auto msg_len = static_cast<int>(value.ByteSizeLong());
    value.SerializeToArray(payload.data(), msg_len);
    return TypedMessage(typename_message<T>(), std::move(payload));
  }

  // This is a template specialization for restore_as function for types derived
  // from google::protobuf::Message.
  template <protobuf_message T>
  inline auto restore_as(const TypedMessage& msg) noexcept
      -> restore_result<T> {
    auto check_type_name = [&]() {
      constexpr auto tn = a4z::type_name<T>();
      if (msg.type.size() != tn.size()) {
        return false;
      }
      const auto expected_name = std::string_view(tn.c_str(), tn.size());
      const auto having_name = as_string_view(msg.type);
      return expected_name == having_name;
    };

    auto unexpected = [](std::string_view err_msg) {
      return tl::make_unexpected(std::runtime_error(std::string{err_msg}));
    };

    if (!check_type_name()) {
      return unexpected("Message type does not match");
    }
    T value;
    auto proto_size = static_cast<int>(msg.payload.size());
    if (!value.ParseFromArray(msg.payload.data(), proto_size)) {
      return unexpected("Failed to parse protobuf");
    }
    return value;
  }


}  // namespace zq
