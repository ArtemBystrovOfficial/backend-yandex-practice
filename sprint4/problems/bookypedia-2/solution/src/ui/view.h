#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>
#include "../app/use_cases.h"

namespace menu {
class Menu;
}

namespace ui {

namespace detail = app::detail;

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool DeleteAuthor(std::istream& cmd_input) const;
    bool DeleteBook(std::istream& cmd_input) const;
    bool EditAuthor(std::istream& cmd_input) const;
    bool EditBook(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowBook(std::istream& cmd_input) const;
    bool ShowAuthorBooks() const;

    std::vector<std::string> ParseTags(const std::string & tags_raw) const;
    std::vector<std::string> GetTags() const;
    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    std::optional<std::string> SelectAuthor() const;
    std::optional<detail::BookInfo> SelectBook() const;
    std::optional<detail::BookInfo> SelectBookOneOf(const std::vector<detail::BookInfo> & books) const;
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::vector<detail::BookInfo> GetBooks() const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui