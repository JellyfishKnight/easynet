#pragma once

namespace net {

#define NET_DECLARE_PTRS(CLASS) \
    using SharedPtr = std::shared_ptr<CLASS>; \
    using ConstSharedPtr = std::shared_ptr<const CLASS>; \
    using WeakPtr = std::weak_ptr<CLASS>; \
    using ConstWeakPtr = std::weak_ptr<const CLASS>; \
    using UniquePtr = std::unique_ptr<CLASS>; \
    using ConstUniquePtr = std::unique_ptr<const CLASS>;

} // namespace net