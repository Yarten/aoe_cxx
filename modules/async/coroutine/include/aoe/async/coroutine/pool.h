//
// Created by yarten on 23-12-27.
//

#pragma once

#include <memory>

#include "./base.h"
#include "./pipe.h"


namespace aoe::async::coroutine
{
	template<class H>
	concept GoFuncHandlerTrait = requires(H && h)
	{
		{ h.intoHandle() } -> std::same_as<std::coroutine_handle<Base>>;
	};

	template<class F, class ... TArgs>
	concept GoFuncTrait = requires(F && func, TArgs &&... args)
	{
		{ func(std::forward<TArgs>(args)...) } -> GoFuncHandlerTrait;
	};

	class Pool
    {
		class Impl;
	public:
		Pool();
		Pool(const Pool &) = delete;
		Pool(Pool &&) = delete;
		Pool & operator=(const Pool &) = delete;
		Pool & operator=(Pool &&) = delete;

    public:
        template<class T, class ... TArgs>
        Pipe<T> makePipe(TArgs &&... args)
        {
			return {lifetime_, std::forward<TArgs>(args)...};
        }

		template<class F, class ... TArgs> requires(GoFuncTrait<F, TArgs...>)
		void operator()(F && go_func, TArgs &&... args)
        {
        	const auto curr_h = switchTo({});
			suspendCurrentInitPoint(true);

        	add(go_func(std::forward<TArgs>(args)...).intoHandle());

        	switchTo(curr_h);
        	suspendCurrentInitPoint(false);
        }

	private:
		friend void awake(std::weak_ptr<Pool> _pool, std::coroutine_handle<Base> handle);

		void add(std::coroutine_handle<Base> handle);

    private:
		std::shared_ptr<Pool> lifetime_;
	    std::unique_ptr<Impl> impl_;
    };
}
