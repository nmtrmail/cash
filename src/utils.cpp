#include <stdarg.h>
#include <cxxabi.h>
#include <regex>
#include "common.h"
#include "platform.h"

using namespace ch::internal;

std::string ch::internal::stringf(const char* format, ...) {
  static constexpr uint32_t STACK_BUFFER_SIZE = 1024;

  std::string result;
  char stack_buffer[STACK_BUFFER_SIZE];
  auto buffer = stack_buffer;
  bool is_heap_buffer = false;

  va_list args_orig, args_copy;
  va_start(args_orig, format);

  va_copy(args_copy, args_orig);
  size_t size = vsnprintf(buffer, STACK_BUFFER_SIZE, format, args_copy) + 1;
  va_end(args_copy);

  if (size > STACK_BUFFER_SIZE) {
    buffer = new char[size];
    is_heap_buffer = true;
    va_copy(args_copy, args_orig);
    vsnprintf(buffer, size, format, args_copy);
    va_end(args_copy);
  }

  va_end(args_orig);

  result.assign(buffer);
  if (is_heap_buffer)
    delete[] buffer;

  return result;
}

std::vector<std::string> ch::internal::split(const std::string& str, char delimiter) {
  std::vector<std::string> out;
  size_t pos = 0;
  while (pos < str.size()) {
    auto index = str.find(delimiter, pos);
    if (index == std::string::npos)
      break;
    out.push_back(str.substr(pos, index - pos));
    pos = index + 1;
  }
  out.push_back(str.substr(pos));
  return out;
}


void ch::internal::dbprint(int level, const char* format, ...) {
  if (level > platform::self().dbg_level())
    return;
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

std::string ch::internal::demanged_typeid(const std::string& name) {
  int status;
  char* demangled = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
  CH_CHECK(0 == status, "abi::__cxa_demangle() failed");
  std::string sd(demangled);
  ::free(demangled);
  return sd;
}

std::string ch::internal::identifier_from_string(const std::string& name) {
  auto ret(name);
  auto not_identifier = [&](auto x){ return !isalnum(x) && (x != '_'); };
  std::replace_if(ret.begin(), ret.end(), not_identifier, '_');
  return ret;
}

std::string ch::internal::identifier_from_typeid(const std::string& name,
                                                 bool remove_template_params) {
  std::string s_out, s_in;

  auto it_out = std::back_inserter(s_out);
  s_in = demanged_typeid(name);

  if (remove_template_params) {
    std::regex_replace(it_out, s_in.begin(), s_in.end(), std::regex("<.*>"), "");
    std::swap(s_out, s_in);
    s_out.clear();
  }

  // remove all spaces, close brakets and parenthesis
  std::regex_replace(it_out, s_in.begin(), s_in.end(), std::regex("[\\s>\\(\\)]+"), "");
  std::swap(s_out, s_in);
  s_out.clear();

  // remove all namespaces
  std::regex_replace(it_out, s_in.begin(), s_in.end(), std::regex("[a-zA-Z_][a-zA-Z0-9_]*::"), "");
  std::swap(s_out, s_in);
  s_out.clear();

  // replace template open brackets and commas
  std::regex_replace(it_out, s_in.begin(), s_in.end(), std::regex("[<,]"), "_");

  return s_out;
}

int ch::internal::char2int(char x, int base) {
  switch (base) {
  case 2:
    if (x >= '0' && x <= '1')
      return (x - '0');
    break;
  case 8:
    if (x >= '0' && x <= '7')
      return (x - '0');
    break;
  case 16:
    if (x >= '0' && x <= '9')
      return (x - '0');
    if (x >= 'A' && x <= 'F')
      return (x - 'A') + 10;
    if (x >= 'a' && x <= 'f')
      return (x - 'a') + 10;
    break;
  }
  CH_ABORT("invalid value");
}
