#pragma once

class UServiceLocator
{
  public:
	template <typename T_>
	auto static Provide(T_ &Service) WIND_NOEXCEPT->void
	{
		instance<T_>() = Service;
	}

	template <typename T_>
	auto static Provide(T_ *Service) WIND_NOEXCEPT->void
	{
		Instance<T_>() = Service;
	}

	template <typename T_>
	static auto Get() WIND_NOEXCEPT -> T_ &
	{
		T_ *Service = Instance<T_>();

		WIND_ASSERT(Service && "Service has not been registered");

		return *Service;
	}

  private:
	template <typename T_>
	static auto Instance() -> T_ *&
	{
		static T_ *SPtr = nullptr;
		return SPtr;
	}
};
