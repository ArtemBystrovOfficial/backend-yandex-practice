#pragma once

#include <vector>
#include <string>
#include <optional>
#include "../unit/unit_of_work.h"

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
    std::string author_name;
    std::string id;
};

}  // namespace detail

class UseCases {
public:

    using authors_list_t = std::vector<detail::AuthorInfo>;
    using books_list_t = std::vector<detail::BookInfo>;
    using tag_list_t = std::vector<std::string>;

    virtual void Commit() {}
    virtual void Rollback() {}

    virtual void EditBook(const std::string & book_id, const std::string & title, int publication_year, const std::vector<std::string> & tags) = 0;
    virtual void EditAuthorName(const std::string & author_id, const std::string & author_new_name) = 0;
    virtual void DeleteAuthorAndDependenciesByName(const std::string & author_name) = 0;
    virtual void DeleteAuthorAndDependencies(const std::string & author_id) = 0;
    virtual void DeleteBookAndDependencies (std::string & book_id) = 0;
    virtual std::string AddAuthor(const std::string& name) = 0;
    virtual std::string AddBook(int year, const std::string & author_id, const std::string& title) = 0;
    virtual void AddTags(const std::string & book_id,const std::vector<std::string> & tags) = 0;

    virtual authors_list_t GetAuthors() = 0;
    virtual books_list_t GetBooks() = 0;
    virtual books_list_t GetBooksAuthors(const std::string & author_id) = 0;
    virtual std::optional<detail::AuthorInfo> FindAuthorByName(const std::string & name) = 0;
    virtual books_list_t FindBooksByTitle(const std::string & title) = 0;
    virtual tag_list_t GetTagsByBookId(const std::string &) = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app
