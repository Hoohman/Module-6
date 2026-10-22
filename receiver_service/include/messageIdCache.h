#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <unordered_set>
#include <utility>

class MessageIdCache {
public:
    bool contains(const std::string& id) const { return ids_.contains(id); }

    void remember(std::string id) {
        if (ids_.contains(id)) {
            return;
        }

        ids_.insert(id);
        order_.push_back(std::move(id));

        if (order_.size() > kMaxIds) {
            ids_.erase(order_.front());
            order_.pop_front();
        }
    }

private:
    static constexpr std::size_t kMaxIds = 1000;

    std::unordered_set<std::string> ids_;
    std::deque<std::string> order_;
};