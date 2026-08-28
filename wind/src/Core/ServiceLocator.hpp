#pragma once

class ServiceLocator
{
public:
  template <typename T>
  auto static provide(T& service) WIND_NOEXCEPT->void
  {
    instance<T>() = service;
  }

  template <typename T>
  auto static provide(T* service) WIND_NOEXCEPT->void
  {
    instance<T>() = service;
  }


  template <typename T>
  static auto get() WIND_NOEXCEPT -> T&
  {
    T* service = instance<T>();

    WIND_ASSERT(service && "Service has not been registered");

    return *service;
  }

private:
  template <typename T>
  static auto instance() -> T*&
  {
    static T* ptr = nullptr;
    return ptr;
  }
};
