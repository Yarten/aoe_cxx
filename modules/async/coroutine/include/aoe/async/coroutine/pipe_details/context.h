//
// Created by yarten on 24-1-7.
//

#pragma once

#include "./one_one_context.h"
#include "./one_multi_context.h"


namespace aoe::async::coroutine::pipe_details
{
    template <class T>
    class Context
    {
    public:
        Context(
            std::shared_ptr<Pool> pool,
            const std::size_t buffer_size,
            const std::size_t senders_count,
            const std::size_t receivers_count,
            const BroadcastType broadcast_type
        )
        {
            assert(senders_count != 0 and receivers_count != 0);

            if (senders_count == 1 and receivers_count == 1)
            {
                core_.template emplace<OneOneContext<T>>(std::move(pool), buffer_size);
            }
            else if (senders_count == 1)
            {
                initCore<OneMultiContext>(broadcast_type, std::move(pool), buffer_size, receivers_count);
            }
            else if (receivers_count == 1)
            {
                panic.todo("MultiOneContext");
            }
            else
            {
                panic.todo("MultiMultiContext");
            }
        }

    public:
        SendId newSend()
        {
            return std::visit(
                trait::impl{
                    [](std::nullptr_t) -> SendId
                    {
                        panic.wtf("");
                    },
                    []<class TContext>(TContext& ctx)
                    {
                        return ctx.newSend();
                    }
                }, core_
            );
        }

        RecvId newRecv()
        {
            return std::visit(
                trait::impl{
                    [](std::nullptr_t) -> RecvId
                    {
                        panic.wtf("");
                    },
                    []<class TContext>(TContext& ctx)
                    {
                        return ctx.newRecv();
                    }
                }, core_
            );
        }

        void deleteSend(const SendId id)
        {
            std::visit(
                trait::impl{
                    [](std::nullptr_t)
                    {
                        panic.wtf("");
                    },
                    [id]<class TContext>(TContext& ctx)
                    {
                        ctx.deleteSend(id);
                    }
                }, core_
            );
        }

        void deleteRecv(const RecvId id)
        {
            std::visit(
                trait::impl{
                    [](std::nullptr_t)
                    {
                        panic.wtf("");
                    },
                    [id]<class TContext>(TContext& ctx)
                    {
                        ctx.deleteRecv(id);
                    }
                }, core_
            );
        }

    private:
        template <class... TContexts>
        struct Trait
        {
            using ContextType = std::variant<std::nullptr_t, TContexts...>;
            using SendAwaiterType = std::variant<std::nullptr_t, typename TContexts::SendAwaiterType...>;
            using RecvAwaiterType = std::variant<std::nullptr_t, typename TContexts::RecvAwaiterType...>;
        };

        template <class E>
        struct TypeSelector;

        template <class E> requires(std::is_copy_assignable_v<E>)
        struct TypeSelector<E>
        {
            using Use = Trait<
                OneOneContext<T>,
                OneMultiContext<T, BroadcastType::None>,
                OneMultiContext<T, BroadcastType::Some>,
                OneMultiContext<T, BroadcastType::All>
            >;
        };

        template <class E> requires(not std::is_copy_assignable_v<E>)
        struct TypeSelector<E>
        {
            using Use = Trait<
                OneOneContext<T>,
                OneMultiContext<T, BroadcastType::None>
            >;
        };

    private:
        template <template<class, BroadcastType> class TContext, class... TArgs>
        void initCore(const BroadcastType broadcast_type, TArgs&&... args)
        {
            switch (broadcast_type)
            {
            case BroadcastType::None:
                core_.template emplace<TContext<T, BroadcastType::None>>(
                    std::forward<TArgs>(args)...
                );
                break;
            case BroadcastType::Some:
                if constexpr (std::is_copy_assignable_v<T>)
                    core_.template emplace<TContext<T, BroadcastType::Some>>(
                        std::forward<TArgs>(args)...
                    );
                else
                    panic.wtf("");
                break;
            case BroadcastType::All:
                if constexpr (std::is_copy_assignable_v<T>)
                    core_.template emplace<TContext<T, BroadcastType::All>>(
                        std::forward<TArgs>(args)...
                    );
                else
                    panic.wtf("");
                break;
            default:
                panic.wtf("");
            }
        }

    private:
        typename TypeSelector<T>::Use::ContextType core_;

    private:
        template <class TDerived, class TAwaiterType>
        class AwaiterBase;

        template <class TDerived, class... TAwiaters>
        class AwaiterBase<TDerived, std::variant<std::nullptr_t, TAwiaters...>>
            : public BoolAwaiter<AwaiterBase<TDerived, std::variant<std::nullptr_t, TAwiaters...>>>
        {
            using Super = BoolAwaiter<AwaiterBase>;

            TDerived& derived() noexcept
            {
                return *static_cast<TDerived*>(this);
            }
        public:
            AwaiterBase()
                : Super(currentHandle())
            {
            }

            TDerived && operator&(std::function<void()> true_callback) &&
            {
                std::move(*this).Super::operator&(std::move(true_callback));
                return std::move(derived());
            }

            TDerived && operator|(std::function<void()> false_callback) &&
            {
                std::move(*this).Super::operator|(std::move(false_callback));
                return std::move(derived());
            }

        public:
            [[nodiscard]] bool isReady() const
            {
                return std::visit(
                    trait::impl{
                        [](std::nullptr_t) { return true; },
                        []<class TAwaiter>(TAwaiter& awaiter) -> bool
                        {
                            return awaiter.isReady();
                        }
                    }, core_
                );
            }

            void onSuspend(const std::coroutine_handle<Base> handle)
            {
                std::visit(
                    trait::impl{
                        [](std::nullptr_t)
                        {
                        },
                        [handle]<class TAwaiter>(TAwaiter& awaiter)
                        {
                            awaiter.onSuspend(handle);
                        }
                    }, core_
                );
            }

            bool onResume()
            {
                return std::visit(
                    trait::impl{
                        [](std::nullptr_t) { return false; },
                        []<class TAwaiter>(TAwaiter& awaiter) -> bool
                        {
                            return awaiter.onResume();
                        }
                    }, core_
                );
            }

            void onAbort()
            {
                std::visit(
                    trait::impl{
                        [](std::nullptr_t)
                        {
                        },
                        []<class TAwaiter>(TAwaiter& awaiter)
                        {
                            awaiter.onAbort();
                        }
                    }, core_
                );
            }

        protected:
            std::variant<std::nullptr_t, TAwiaters...> core_;
        };

    public:
        class SendAwaiter : public AwaiterBase<SendAwaiter, typename TypeSelector<T>::Use::SendAwaiterType>
        {
            using Super = AwaiterBase<SendAwaiter, typename TypeSelector<T>::Use::SendAwaiterType>;

        private:
            friend class Context;

            template <class TMovedOrCopied>
            SendAwaiter(Context& self, const SendId id, TMovedOrCopied&& data)
            {
                std::visit(
                    trait::impl{
                        [](std::nullptr_t)
                        {
                        },
                        [&]<class TContext>(TContext& ctx)
                        {
                            Super::core_.template emplace<typename TContext::SendAwaiterType>(
                                ctx.send(id, std::forward<TMovedOrCopied>(data))
                            );
                        }
                    }, self.core_
                );
            }
        };

        class RecvAwaiter : public AwaiterBase<RecvAwaiter, typename TypeSelector<T>::Use::RecvAwaiterType>
        {
            using Super = AwaiterBase<RecvAwaiter, typename TypeSelector<T>::Use::RecvAwaiterType>;

        private:
            friend class Context;

            RecvAwaiter(Context& self, const RecvId id, T& result)
            {
                std::visit(
                    trait::impl{
                        [](std::nullptr_t)
                        {
                        },
                        [&]<class TContext>(TContext& ctx)
                        {
                            Super::core_.template emplace<typename TContext::RecvAwaiterType>(
                                ctx.recv(id, result)
                            );
                        }
                    }, self.core_
                );
            }
        };

    public:
        template <class TMovedOrCopied>
        SendAwaiter send(const SendId id, TMovedOrCopied&& data)
        {
            return {*this, id, std::forward<TMovedOrCopied>(data)};
        }

        RecvAwaiter recv(const RecvId id, T& result)
        {
            return {*this, id, result};
        }
    };
}
