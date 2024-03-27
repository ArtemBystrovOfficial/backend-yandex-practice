#pragma once
#include <string>
#include "book.h"

namespace domain {

class Tag {
public:
    Tag(BookId book_id, std::string tag)
        : book_id_(std::move(book_id))
        , tag_(std::move(tag)) {
    }

    const BookId& GetBookId() const noexcept {
        return book_id_;
    }

    const std::string& GetTag() const noexcept {
        return tag_;
    }

private:
    BookId book_id_;
    std::string tag_;
};

class TagRepository {
public:
    using list_tags_t = std::vector<Tag>;

    virtual void ClearTagsByBookId(const BookId & book) = 0;
    virtual void Save(const Tag& tag) = 0;
    virtual list_tags_t GetTagsByBookId(const BookId & book) = 0;

protected:
    ~TagRepository() = default;
};

}  // namespace domain
