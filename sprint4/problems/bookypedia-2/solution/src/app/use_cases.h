#pragma once

#include <vector>
#include <string>

namespace app {

namespace detail {

struct AddBookParams {
    std::string title;
    std::string author_id;
    int publication_year = 0;
};

struct AuthorInfo {
    std::string id;
    std::string name;
};

struct BookInfo {
    std::string title;
    int publication_year;
};

}  // namespace detail

class UseCases {
public:
    using authors_list_t = std::vector<detail::AuthorInfo>;
    using books_list_t = std::vector<detail::BookInfo>;

    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(int year, const std::string & author_id, const std::string& title) = 0;

    virtual authors_list_t GetAuthors() = 0;
    virtual books_list_t GetBooks() = 0;
    virtual books_list_t GetBooksAuthors(const std::string & author_id) = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app
