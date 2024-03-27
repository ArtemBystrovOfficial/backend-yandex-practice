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
    Book(BookId id, Author author, std::string title, int year)
        : id_(std::move(id))
        , title_(std::move(title))
        , author_(std::move(author))
        , year_(year) {
    }

    const BookId& GetId() const noexcept {
        return id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return author_.GetId();
    }

    const std::string & GetAuthorName() const noexcept {
        return author_.GetName();
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    int GetYear() const noexcept {
        return year_;
    }

private:
    BookId id_;
    domain::Author author_; //TODO std::variant <id, author>
    std::string title_;
    int year_;
};

class BookRepository {
public:
    using list_books_t = std::vector<Book>;

    virtual void Save(const Book& Book) = 0;
    virtual void Edit(const Book& Book) = 0;
    virtual void Delete(const BookId& Book) = 0;
    virtual list_books_t GetList() = 0;
    virtual list_books_t GetBookByAuthorId(const AuthorId &) = 0;
    virtual list_books_t GetBooksByTitle(const std::string &) = 0;

protected:
    ~BookRepository() = default;
};

}  // namespace domain
