#include "postgres.h"

#include <pqxx/zview.hxx>
#include <pqxx/pqxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

domain::AuthorRepository::list_authors_t AuthorRepositoryImpl::GetList() { 
    pqxx::read_transaction r(connection_);
    domain::AuthorRepository::list_authors_t authors_list;

    auto query_text = "SELECT * from authors ORDER BY name ASC;"_zv;
    for(auto [id, name] : r.query<std::string, std::string>(query_text)) {
        authors_list.push_back(domain::Author(domain::AuthorId::FromString(id), name));
    }
    return authors_list;
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4);
)"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetYear());
    work.commit();
}

domain::BookRepository::list_books_t BookRepositoryImpl::GetList() { 
    pqxx::read_transaction r(connection_);
    domain::BookRepository::list_books_t books_list;
    auto query_text = "SELECT * from books ORDER BY title ASC;"_zv;
    for(auto [id, author_id, title, year] : r.query<std::string, std::string, std::string, int>(query_text)) {
        books_list.push_back(domain::Book(domain::BookId::FromString(id), domain::AuthorId::FromString(author_id), title, year));
    }
    return books_list;
}

domain::BookRepository::list_books_t BookRepositoryImpl::GetBookByAuthorId(const domain::AuthorId& author_id) {
    pqxx::read_transaction r(connection_);
    domain::BookRepository::list_books_t books_list;
    auto query_text = "SELECT * FROM books WHERE author_id = " + r.quote(author_id.ToString()) + " ORDER BY publication_year ASC, title ASC;";
    for(auto [id, author_id, title, year] : r.query<std::string, std::string, std::string, int>(pqxx::zview(query_text))) {
        books_list.push_back(domain::Book(domain::BookId::FromString(id), domain::AuthorId::FromString(author_id), title, year));
    }
    return books_list; 
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);


    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL,
    title varchar(100) NOT NULL,
    publication_year int NOT NULL,
    CONSTRAINT books_authors FOREIGN KEY (author_id) REFERENCES authors (id)
);
)"_zv);
    work.commit();
}

}  // namespace postgres