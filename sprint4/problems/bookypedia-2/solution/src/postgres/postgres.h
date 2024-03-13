#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"
#include "../domain/tag.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::work& worker)
        : worker_{worker} {
    }

    void DeleteAuthorAndDependencies(const domain::Author& author) override;
    void Save(const domain::Author& author) override;
    list_authors_t GetList() override;
    std::optional <domain::Author> FindAuthorByName(const std::string & name) override;

private:
    pqxx::work& worker_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::work& worker)
        : worker_{worker} {        
    }

    void Delete(const domain::BookId& Book) override;
    void Edit(const domain::Book& Book) override;
    void Save(const domain::Book& book) override;
    list_books_t GetList() override;
    list_books_t GetBookByAuthorId(const domain::AuthorId &) override;
    list_books_t GetBooksByTitle(const std::string &) override;

private:
    pqxx::work& worker_;
};

class TagRepositoryImpl : public domain::TagRepository {
public:
    explicit TagRepositoryImpl(pqxx::work& worker)
        : worker_{worker} {        
    }

    void ClearTagsByBookId(const domain::BookId & book_id) override;
    void Save(const domain::Tag& tag) override;
    list_tags_t GetTagsByBookId(const domain::BookId & book) override;
private:
    pqxx::work& worker_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    pqxx::connection & GetConnection() { return connection_; }

private:
    pqxx::connection connection_;
};

}  // namespace postgres