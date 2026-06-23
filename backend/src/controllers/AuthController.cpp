#include "../include/controllers/AuthController.h"
#include "../include/models/User.h"
#include <crow.h>
#include <pqxx/pqxx>
#include <jwt-cpp/jwt.h>
#include <bcrypt/BCrypt.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
using namespace std;
using json = nlohmann::json;

string AuthController::hashPassword(string plainText) {
    return BCrypt::generateHash(plainText);
}

bool AuthController::checkPassword(string plainText, string hash) {
    return BCrypt::validatePassword(plainText, hash);
}

string AuthController::createToken(int userId, string role) {
    auto token = jwt::create()
                     .set_issuer("shiftwise")
                     .set_type("JWT")
                     .set_payload_claim("userId", jwt::claim(to_string(userId)))
                     .set_payload_claim("role",   jwt::claim(role))
                     .set_expires_at(chrono::system_clock::now() + chrono::hours{24})
                     .sign(jwt::algorithm::hs256{jwtSecret});
    return token;
}

// ─────────────────────────────────────────────
//  POST /api/auth/signup
// ─────────────────────────────────────────────
crow::response AuthController::signup(const crow::request& req) {
    try {
        json body = json::parse(req.body);

        string username   = body.value("username",   "");
        string email      = body.value("email",      "");
        string password   = body.value("password",   "");
        string first_name = body.value("first_name", "");
        string last_name  = body.value("last_name",  "");
        string role       = body.value("role",       "employee");

        // Basic validation
        if (username.empty() || email.empty() || password.empty() ||
            first_name.empty() || last_name.empty()) {
            return crow::response(400, "{\"error\":\"All fields are required\"}");
        }

        // Force role to only be employee or manager — nothing else
        if (role != "employee" && role != "manager") {
            role = "employee";
        }

        // Managers need approval, employees are active immediately
        string status = (role == "manager") ? "pending" : "active";

        pqxx::connection conn(dbConnection);
        pqxx::work txn(conn);

        // Check email
        auto existingEmail = txn.exec_params(
            "SELECT id FROM users WHERE email = $1", email
        );
        if (!existingEmail.empty()) {
            return crow::response(409, "{\"error\":\"Email already registered\"}");
        }

        // Check username
        auto existingUsername = txn.exec_params(
            "SELECT id FROM users WHERE username = $1", username
        );
        if (!existingUsername.empty()) {
            return crow::response(409, "{\"error\":\"Username already taken\"}");
        }

        string hashedPwd = hashPassword(password);

        auto result = txn.exec_params(
            "INSERT INTO users (username, email, password_hash, first_name, last_name, role, status) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING id",
            username, email, hashedPwd, first_name, last_name, role, status
        );

        txn.commit();

        // If manager, return pending message — no token yet
        if (role == "manager") {
            json response;
            response["message"] = "Manager account request submitted. Awaiting admin approval.";
            response["status"]  = "pending";
            return crow::response(202, response.dump());
        }

        // Employee gets token immediately
        string newId = result[0][0].as<string>();
        string token = createToken(0, role); // passing 0 as placeholder, adjust if needed

        json response;
        response["message"] = "Account created successfully";
        response["token"]   = token;

        return crow::response(201, response.dump());

    } catch (exception& e) {
        cerr << "Signup error: " << e.what() << endl;
        return crow::response(500, "{\"error\":\"Internal server error\"}");
    }
}

// ─────────────────────────────────────────────
//  POST /api/auth/login
// ─────────────────────────────────────────────
crow::response AuthController::login(const crow::request& req) {
    try {
        json body = json::parse(req.body);

        string email    = body.value("email",    "");
        string password = body.value("password", "");

        if (email.empty() || password.empty()) {
            return crow::response(400, "{\"error\":\"Email and password are required\"}");
        }

        pqxx::connection conn(dbConnection);
        pqxx::work txn(conn);

        auto result = txn.exec_params(
            "SELECT id, username, email, password_hash, role, status FROM users WHERE email = $1",
            email
        );

        if (result.empty()) {
            return crow::response(401, "{\"error\":\"Invalid email or password\"}");
        }

        string userId   = result[0][0].as<string>();
        string userName = result[0][1].as<string>();
        string userHash = result[0][3].as<string>();
        string userRole = result[0][4].as<string>();
        string userStatus = result[0][5].as<string>();

        // ── STATUS CHECKS ──
        if (userStatus == "pending") {
            return crow::response(403, "{\"error\":\"Your manager account is awaiting admin approval.\"}");
        }

        if (userStatus == "rejected") {
            return crow::response(403, "{\"error\":\"Your account request was rejected. Contact admin.\"}");
        }

        if (!checkPassword(password, userHash)) {
            return crow::response(401, "{\"error\":\"Invalid email or password\"}");
        }

        string token = createToken(0, userRole); // adjust userId if needed

        json response;
        response["message"]  = "Login successful";
        response["token"]    = token;
        response["user"]["username"] = userName;
        response["user"]["email"]    = email;
        response["user"]["role"]     = userRole;

        return crow::response(200, response.dump());

    } catch (exception& e) {
        cerr << "Login error: " << e.what() << endl;
        return crow::response(500, "{\"error\":\"Internal server error\"}");
    }
}

// ─────────────────────────────────────────────
//  GET /api/auth/pending-managers
//  Admin only — see who's waiting for approval
// ─────────────────────────────────────────────
crow::response AuthController::getPendingManagers(const crow::request& req) {
    try {
        pqxx::connection conn(dbConnection);
        pqxx::work txn(conn);

        auto result = txn.exec(
            "SELECT id, username, email, first_name, last_name, created_at "
            "FROM users WHERE role = 'manager' AND status = 'pending' "
            "ORDER BY created_at ASC"
        );

        json users = json::array();
        for (auto row : result) {
            json u;
            u["id"]         = row[0].as<string>();
            u["username"]   = row[1].as<string>();
            u["email"]      = row[2].as<string>();
            u["first_name"] = row[3].as<string>();
            u["last_name"]  = row[4].as<string>();
            u["created_at"] = row[5].as<string>();
            users.push_back(u);
        }

        return crow::response(200, users.dump());

    } catch (exception& e) {
        cerr << "getPendingManagers error: " << e.what() << endl;
        return crow::response(500, "{\"error\":\"Internal server error\"}");
    }
}

// ─────────────────────────────────────────────
//  PUT /api/auth/approve/:id
//  PUT /api/auth/reject/:id
// ─────────────────────────────────────────────
crow::response AuthController::updateManagerStatus(const crow::request& req, string userId, string newStatus) {
    try {
        pqxx::connection conn(dbConnection);
        pqxx::work txn(conn);

        auto result = txn.exec_params(
            "UPDATE users SET status = $1 WHERE id = $2 AND role = 'manager' RETURNING id",
            newStatus, userId
        );

        txn.commit();

        if (result.empty()) {
            return crow::response(404, "{\"error\":\"Manager not found\"}");
        }

        json response;
        response["message"] = "Manager status updated to " + newStatus;
        return crow::response(200, response.dump());

    } catch (exception& e) {
        cerr << "updateManagerStatus error: " << e.what() << endl;
        return crow::response(500, "{\"error\":\"Internal server error\"}");
    }
}
