#include "postgres.h"

#include <pqxx/zview.hxx>
#include <pqxx/pqxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::DeleteAuthorAndDependencies(const domain::Author& author) {

    if(!author.GetName().empty()) {
        worker_.exec_params( R"( DELETE FROM authors WHERE name = $1; )"_zv,
        author.GetName());
        return;
    }

    worker_.exec_params( R"( DELETE FROM authors WHERE id = $1; )"_zv,
        author.GetId().ToString());
}

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    //pqxx::work work{connection_};
    worker_.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
}

void TagRepositoryImpl::ClearTagsByBookId(const domain::BookId& book_id) {
    worker_.exec_params( R"( DELETE FROM book_tags WHERE book_id = $1; )"_zv, book_id.ToString());
}

void TagRepositoryImpl::Save(const domain::Tag& tag) {
    worker_.exec_params(
        R"(
INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);
)"_zv,
        tag.GetBookId().ToString(), tag.GetTag());
}

domain::TagRepository::list_tags_t TagRepositoryImpl::GetTagsByBookId(const domain::BookId& book) {
    auto query_text = "SELECT book_id, tag FROM book_tags WHERE book_id = " + worker_.quote(book.ToString()) + " ORDER BY tag ASC;";
    domain::TagRepository::list_tags_t list;
    for(auto [book_id, tag] : worker_.query<std::string, std::string>(query_text)) {
        list.push_back({domain::BookId::FromString(book_id), tag});
    }

    return list;
}

domain::AuthorRepository::list_authors_t AuthorRepositoryImpl::GetList() { 
    domain::AuthorRepository::list_authors_t authors_list;

    auto query_text = "SELECT * FROM authors ORDER BY name ASC;"_zv;
    for(auto [id, name] : worker_.query<std::string, std::string>(query_text)) {
        authors_list.push_back(domain::Author(domain::AuthorId::FromString(id), name));
    }
    return authors_list;
}

std::optional <domain::Author> AuthorRepositoryImpl::FindAuthorByName(const std::string & name) {
    auto query_text = "SELECT * FROM authors WHERE name = "s + worker_.quote(name) + ";"s;
    auto author = worker_.query01<std::string, std::string>(pqxx::zview(query_text));
    if(!author) 
        return std::nullopt;
    return domain::Author(domain::AuthorId::FromString(std::get<0>(*author)), std::get<1>(*author)); 
}

void BookRepositoryImpl::Delete(const domain::BookId& book_id) {
    worker_.exec_params( "DELETE FROM books WHERE id = $1; "_zv, book_id.ToString());
}

void BookRepositoryImpl::Edit(const domain::Book& book) {
        worker_.exec_params(
        R"(
UPDATE books SET title = $2, publication_year = $3 WHERE id = $1;
)"_zv,
        book.GetId().ToString(), book.GetTitle(), book.GetYear());
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    worker_.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4);
)"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetYear());
}

domain::BookRepository::list_books_t BookRepositoryImpl::GetList() { 
    domain::BookRepository::list_books_t books_list;
    auto query_text = R"(SELECT books.id, author_id, name, title, publication_year 
                        FROM books 
                        INNER JOIN authors ON books.author_id = authors.id 
                        ORDER BY title, name, publication_year;)"_zv;
    for(auto [id, author_id, author_name, title, year] : worker_.query<std::string, std::string,std::string,  std::string, int>(query_text)) {
        books_list.push_back(domain::Book(domain::BookId::FromString(id), {domain::AuthorId::FromString(author_id), author_name}, title, year));
    }
    return books_list;
}

domain::BookRepository::list_books_t BookRepositoryImpl::GetBookByAuthorId(const domain::AuthorId& author_id) {
    domain::BookRepository::list_books_t books_list;
    auto query_text = "SELECT id, author_id, title, publication_year FROM books WHERE author_id = " + worker_.quote(author_id.ToString()) + 
                      " ORDER BY publication_year ASC, title ASC;";
    for(auto [id, author_id, title, year] : worker_.query<std::string, std::string, std::string, int>(pqxx::zview(query_text))) {
        books_list.push_back(domain::Book(domain::BookId::FromString(id), {domain::AuthorId::FromString(author_id),""}, title, year));
    }
    return books_list; 
}

domain::BookRepository::list_books_t BookRepositoryImpl::GetBooksByTitle(const std::string& title) {
    auto query_text = R"(SELECT books.id, author_id, name, title, publication_year 
                        FROM books 
                        INNER JOIN authors ON books.author_id = authors.id WHERE title = )"
                        + worker_.quote(title) + ";"s;
    domain::BookRepository::list_books_t books_list;
    for(auto [id, author_id, author_name, title, year] : worker_.query<std::string, std::string,std::string,  std::string, int>(query_text)) {
        books_list.push_back(domain::Book(domain::BookId::FromString(id), {domain::AuthorId::FromString(author_id), author_name}, title, year));
    }
   
    return books_list;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL,
    title VARCHAR(100) NOT NULL,
    publication_year int NOT NULL,
    CONSTRAINT books_authors FOREIGN KEY (author_id) REFERENCES authors (id) ON DELETE CASCADE
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID NOT NULL,
    tag VARCHAR(30) NOT NULL,
    CONSTRAINT books FOREIGN KEY (book_id) REFERENCES books (id) ON DELETE CASCADE
);
)"_zv);

    work.commit();
}

}  // namespace postgres