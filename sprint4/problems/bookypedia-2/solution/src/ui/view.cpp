#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <iostream>
#include <boost/regex.hpp>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {

std::ostream& operator<<(std::ostream& out, const detail::AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const detail::BookInfo& book) {
    out << book.title <<" by " << book.author_name << ", " << book.publication_year;
    return out;
}


template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction("AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "[name]"s, "Edit name author"s, std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s, std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("EditBook"s, "[title]"s, "Edit book"s, std::bind(&View::EditBook, this, ph::_1));
    menu_.AddAction("DeleteAuthor"s, "[name]"s, "Cascade delete author"s,std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "[title]"s, "Cascade delete author"s,std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("ShowBook"s, {}, "Show book"s, std::bind(&View::ShowBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,std::bind(&View::ShowAuthorBooks, this));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if(name.empty())
            throw std::runtime_error("empty name");
        use_cases_.AddAuthor(std::move(name));
    } catch (const std::exception& ex) {
        output_ << "Failed to add author"s  << std::endl;
        use_cases_.Rollback();
        return true;
    }
    use_cases_.Commit();
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        if (auto params = GetBookParams(cmd_input)) {
            auto tags = GetTags();
            auto book_id = use_cases_.AddBook(params->publication_year,params->author_id, params->title);
            use_cases_.AddTags(book_id, tags);
        }
    } catch (const std::exception& ex) {
        output_ << "Failed to add book"s +ex.what() << std::endl;
        use_cases_.Rollback();
        return true;
    }
    use_cases_.Commit();
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const { 
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if(name.empty()) {
            auto author_id = SelectAuthor();
            use_cases_.DeleteAuthorAndDependencies(*author_id);
        } else {
            auto author_opt = use_cases_.FindAuthorByName(name);
            if(!author_opt)
                throw std::invalid_argument("Author name not exist");
            use_cases_.DeleteAuthorAndDependenciesByName(name);
        }
    } catch (const std::exception& ex) {
        output_ << "Failed to delete author"s  << std::endl;
        use_cases_.Rollback();
        return true;
    }
    use_cases_.Commit();
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const { 
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        detail::BookInfo book;
        if(title.empty()) {
            auto book_opt = SelectBook();
            if(book_opt.has_value())
                book = *book_opt;
            else
                return true;
        } else {
            auto books = use_cases_.FindBooksByTitle(title);
            if(books.empty())
                throw std::invalid_argument("Book title not exist");
            auto book_opt = SelectBookOneOf(books);
            if(!book_opt) {
                return true;
            }
            book = *book_opt;
        }
        use_cases_.DeleteBookAndDependencies(book.id);
    } catch (const std::invalid_argument& ex) {
        output_ << "Book not found"s  << std::endl;
        use_cases_.Rollback();
        return true;
    } catch (...) {
        output_ << "Failed to delete book"s  << std::endl;
        use_cases_.Rollback();
        return true;
    }
    use_cases_.Commit();
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        std::string author_id;
        if(name.empty()) {
            author_id = SelectAuthor().value();
        } else {
            auto author_opt = use_cases_.FindAuthorByName(name);
            if(!author_opt)
                throw std::invalid_argument("Author name not exist");
            author_id = author_opt->id;
        }

        output_ << "Enter new name:" << std::endl;
        std::string new_name;
        std::getline(input_, new_name);
        boost::algorithm::trim(new_name);

        use_cases_.EditAuthorName(author_id, new_name);

    } catch (const std::exception& ex) {
        output_ << "Failed to edit author"s  << std::endl;
        use_cases_.Rollback();
        return true;
    }
    use_cases_.Commit();
    return true; 
}

bool View::EditBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        detail::BookInfo book;
        if(title.empty()) {
            book = SelectBook().value();
        } else {
            auto books = use_cases_.FindBooksByTitle(title);
            if(books.empty())
                throw std::invalid_argument("Book title not exist");
            auto book_opt = SelectBookOneOf(books);
            if(!book_opt)
                throw std::invalid_argument("Author name not exist");
            book = *book_opt;
        }

        output_ << "Enter new title or empty line to use the current one (" << book.title << "):" << std::endl;
        std::string new_title;
        std::getline(input_, new_title);
        boost::algorithm::trim(new_title);
        if(new_title.empty())
            new_title = book.title;

        output_ << "Enter publication year or empty line to use the current one (" << book.publication_year << "):" << std::endl;
        std::string new_year;
        std::getline(input_, new_year);
        boost::algorithm::trim(new_year);
        auto new_year_value = std::atoi(new_year.c_str());
        if(new_year.empty())
            new_year_value = book.publication_year;

        output_ << "Enter tags ( current tags: " << boost::algorithm::join(use_cases_.GetTagsByBookId(book.id), ", ") << "):" << std::endl;

        std::string tags;
        std::getline(input_, tags);
        boost::algorithm::trim(tags);

        use_cases_.EditBook(book.id,new_title, new_year_value, ParseTags(tags));
    } catch (const std::exception& ex) {
        output_ << "Book not found"s  << std::endl;
        use_cases_.Rollback();
        return true;
    }
    use_cases_.Commit();
    return true; 
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        detail::BookInfo book;

        if(title.empty()) {
            book = *SelectBook();
        } else {
            auto books = use_cases_.FindBooksByTitle(title);
            if(books.empty())
                throw std::invalid_argument("Book title not exist");
            auto book_opt = SelectBookOneOf(books);
            if(!book_opt)
                throw std::invalid_argument("Book title not exist");
            book = *book_opt;
        }

        output_ << "Title: " << book.title << std::endl;
        output_ << "Author: " << book.author_name << std::endl;
        output_ << "Publication year: " << book.publication_year << std::endl;
        auto tags = use_cases_.GetTagsByBookId(book.id);
        if(!tags.empty())
            output_ << "Tags: " << boost::algorithm::join(tags, ", ") << std::endl;
    } catch (const std::exception& ex) {
        //return false;
    }
    return true;
}

bool View::ShowAuthorBooks() const {
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::exception& ex) {
        throw std::runtime_error("Failed to Show Books"s );
    }
    return true;
}

std::vector<std::string> View::ParseTags(const std::string& tags_raw) const {
    boost::regex reg("\\s{2,}");
    auto tags_without_extra_spaces = boost::regex_replace(tags_raw, reg, " ");

    std::vector<std::string> tokens;
    
    boost::split(tokens, tags_without_extra_spaces, boost::is_any_of(","));

    for(auto & token : tokens) {
        boost::algorithm::trim(token);
    }

    tokens.erase(std::remove_if(tokens.begin(), tokens.end(),
                            [](const std::string& s) { return s.empty(); }),
             tokens.end());

    std::sort(tokens.begin(), tokens.end());
    auto last = std::unique(tokens.begin(), tokens.end());
    tokens.erase(last, tokens.end());

    return tokens;
}

std::vector<std::string> View::GetTags() const {
    output_ << "Enter tags (comma separated):" << std::endl;
    
    std::string tags;
    std::getline(input_, tags);

    return ParseTags(tags);
}

std::optional<detail::AddBookParams> View::GetBookParams(
    std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    std::string author_name;
    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::getline(input_, author_name);
    boost::algorithm::trim(author_name); 

    if(author_name.empty()) {
        auto author_id = SelectAuthor();
        if (not author_id.has_value())
            return std::nullopt;
        else {
            params.author_id = author_id.value();
            return params;
        }
    }

    auto author_opt = use_cases_.FindAuthorByName(author_name);
    if(author_opt) {
        params.author_id = author_opt->id;
        return params;
    } else {
        std::string command;
        output_ << "No author found. Do you want to add " << author_name << " (y/n)?" << std::endl;
        std::getline(input_, command);
        boost::algorithm::trim(command);
        if(command == "y" || command == "Y") {
            params.author_id = use_cases_.AddAuthor(author_name);
            return params;
        } else 
            throw std::invalid_argument("Author can't bind");

    }
}

std::optional<std::string> View::SelectAuthor() const {
    output_ << "Select author:" << std::endl;
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx].id;
}

std::optional<detail::BookInfo> View::SelectBook() const {
    return SelectBookOneOf(GetBooks());
}

std::optional<detail::BookInfo> View::SelectBookOneOf( const std::vector<detail::BookInfo>& books) const {
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return books[book_idx];
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    return use_cases_.GetAuthors();
}

std::vector<detail::BookInfo> View::GetBooks() const {
    return use_cases_.GetBooks();
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    return use_cases_.GetBooksAuthors(author_id);
}

}  // namespace ui
