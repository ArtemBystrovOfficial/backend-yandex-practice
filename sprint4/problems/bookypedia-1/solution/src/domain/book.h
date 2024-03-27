#pragma once
#include <string>

#include "../util/tagged_uuid.h"
#include "author.h"

namespace domain {

namespace detail {
struct BookTag {};
}  // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

class Book {
public:
    Book(BookId id, AuthorId author_id, std::string title, int year)
        : id_(std::move(id))
        , title_(std::move(title))
        , author_id_(std::move(author_id))
        , year_(year) {
    }

    const BookId& GetId() const noexcept {
        return id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return author_id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    int GetYear() const noexcept {
        return year_;
    }

private:
    BookId id_;
    AuthorId author_id_;
    std::string title_;
    int year_;
};

class BookRepository {
public:
    using list_books_t = std::vector<Book>;

    virtual void Save(const Book& Book) = 0;
    virtual list_books_t GetList() = 0;
    virtual list_books_t GetBookByAuthorId(const AuthorId &) = 0;

protected:
    ~BookRepository() = default;
};

}  // namespace domain
