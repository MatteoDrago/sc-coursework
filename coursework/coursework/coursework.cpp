// coursework.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <math.h>
#include <vector>
#include <string>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <map>

using namespace std;
using namespace std::chrono;

struct entry {
	int user;
	int item;
	int rating;
	int avg = 0;
};

vector<entry> query(const char* query, sqlite3 *db)
{
	sqlite3_stmt *statement;
	vector<entry> entries;
	int row_counter = 0;

	if (sqlite3_prepare_v2(db, query, -1, &statement, 0) == SQLITE_OK)
	{
		int cols = sqlite3_column_count(statement);
		//cout << "Number of cols: " << cols << std::endl;
		int result = 0;
		while (true)
		{
			result = sqlite3_step(statement);
			if (result == SQLITE_ROW)
			{
				row_counter++;
				entry row_entry;
				for (int i = 0; i < cols; i++)
				{
					switch (i)
					{
					case(0):
						row_entry.user = sqlite3_column_int(statement, i);
						break;
					case(1):
						row_entry.item = sqlite3_column_int(statement, i);
						break;
					case(2):
						row_entry.rating = sqlite3_column_int(statement, i);
						break;
					}
				}
				entries.push_back(row_entry);
			}
			else
			{
				break;
			}
		}
	}
	//cout << "Rows number: " << row_counter << std::endl;
	sqlite3_finalize(statement);
	return entries;
}

double avg(vector<entry> v)
{
	double sum = 0;
	double n = 0;
	for (vector<entry>::iterator it = v.begin(); it != v.end(); ++it)
	{
		sum += it->rating;
		n++;
	}
	return sum / n;
	//return 1.0 * std::accumulate(v.begin(), v.end(), 0LL) / v.size();
	//return accumulate(v.begin(), v.end(), 0.0 / v.size());
}

double avg_value(const char* query, sqlite3 *db) {
	sqlite3_stmt *statement;
	double avg;
	if (sqlite3_prepare_v2(db, query, -1, &statement, 0) == SQLITE_OK)
	{
		int cols = sqlite3_column_count(statement);
		int result = 0;
		while (true)
		{
			result = sqlite3_step(statement);
			if (result == SQLITE_ROW)
			{
				avg = sqlite3_column_double(statement, 0);
			}
			else
			{
				break;
			}
		}
	}
	sqlite3_finalize(statement);
	return avg;
}


bool compareItems(const entry &a, const entry &b)
{
	return a.item < b.item || a.item == b.item && a.user < b.user;
}

int callback(void *data, int argc, char **argv, char **azColName) {
	int i;
	fprintf(stderr, "%s: ", (const char*)data);

	for (i = 0; i < argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}

	printf("\n");
	return 0;
}

int runTransaction(sqlite3 *db, vector<std::string> insert_query)

{

	if (!db)

		return 0;

	char *zErrMsg = 0;

	// start sqlite transaction

	sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);

	for (int i = 0; i < insert_query.size(); i++)
	{
		const char *query = insert_query[i].c_str();
		//cout << query << std::endl;
		int rc = sqlite3_exec(db, query, NULL, 0, &zErrMsg);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
			// rollback all update/insert to sqlite 
			sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
			return 0; // if an error occur
		}

	}
	// commit all to sqlite database
	sqlite3_exec(db, "COMMIT", 0, 0, 0);
	return 1;

}

/*
int getSimilarityRows(sqlite3 *db) {
	int rows;
	char *query = "SELECT COUNT(user1) FROM similarity;";
	sqlite3_stmt *statement;

	if (sqlite3_prepare_v2(db, query, -1, &statement, 0) == SQLITE_OK)
	{
		int result = sqlite3_step(statement);
		if (result == SQLITE_ROW)
		{
			rows = sqlite3_column_int(statement, 0);
		}
		else {
			rows = 0;
		}
	}
	sqlite3_finalize(statement);
	return rows;
}

int getSimilarityColumns(sqlite3 *db) {
	sqlite3_stmt *pStmt;
	const char *tail;
	char *columns_query = "SELECT * from similarity;";
	sqlite3_prepare_v2(db, columns_query, strlen(columns_query), &pStmt, &tail);
	return sqlite3_column_count(pStmt) - 1;
}

double pearson_similarity(int u1, int u2, sqlite3 *db)
{
	if (u1 == u2)
	{
		//cout << "Pearson coefficient: 1" << std::endl;
		return 1.0;
	}

	// The following query returns all the rows relatives to two different users
	string select = "SELECT * from usersratings WHERE usersratings.user in (" + std::to_string(u1) + "," + to_string(u2) + ")";
	vector<entry> users = query(select.c_str(), db);

	vector<entry> user1;
	vector<entry> user2;

	// Split the resulting array between user 1 and user 2
	for (int i = 0; i < users.size(); i++)
	{
		if (users[i].user == u1)
		{
			user1.push_back(users[i]);
		}
		else {
			user2.push_back(users[i]);
		}
	}

	vector<entry> temp1 = user1;
	vector<entry> temp2 = user2;

	double avg1 = avg(user1);
	double avg2 = avg(user2);

	user2.insert(user2.end(), user1.begin(), user1.end());
	std::sort(user2.begin(), user2.end(), compareItems);

	int matches = 0;
	double num = 0;
	double den1 = 0;
	double den2 = 0;

	for (int i = 0; i < user2.size() - 1; i++)
	{
		if (user2[i].item == user2[i + 1].item)
		{
			double avg_int1;
			double avg_int2;
			if (user2[i].user == u1)
			{
				avg_int1 = avg1;
			}
			else {
				avg_int1 = avg2;
			}
			if (user2[i + 1].user == u1)
			{
				avg_int2 = avg1;
			}
			else {
				avg_int2 = avg2;
			}
			double diff1 = (user2[i].rating - avg_int1);
			double diff2 = (user2[i + 1].rating - avg_int2);
			num += diff1 * diff2;
			den1 += pow(diff1, 2);
			den2 += pow(diff2, 2);
			matches++;
		}
	}

	//cout << "Pearson coefficient: " << num / (sqrt(den1) * sqrt(den2)) << std::endl;
	return num / (sqrt(den1) * sqrt(den2));
}

void similarity_matrix(sqlite3 *db, sqlite3 *similarity, long int users, long int interval) {

	int trans_number = 1;
	long int n = 1;
	double sim; // pearson coefficient (from -1 to 1, if NaN we set it to a value of 10)
	char *zErrMsg = 0;
	std::string temp_s;
	vector<std::string> insert_query;

	cout << "Starting creating similarity matrix..." << std::endl;

	for (int i = 1; i <= users; i++)
	{

		sim = pearson_similarity(1, i, db);
		if (isnan(sim)) {
			sim = 10;
		}

		temp_s = "INSERT INTO similarity(userID,user";
		temp_s.append(std::to_string(1).c_str());
		temp_s.append(") VALUES(");
		temp_s.append(std::to_string(i).c_str());
		temp_s.append(",");
		temp_s.append(std::to_string(sim).c_str());
		temp_s.append(");");
		const char *query = temp_s.c_str();
		insert_query.push_back(query);


		if ((n % interval) == 0)
		{
			cout << "Transaction number " << trans_number << std::endl;
			if (runTransaction(similarity, insert_query) == 0) {
				break;
			}
			insert_query.clear();
			trans_number++;
		}
		n++;
	}
	if (insert_query.size() > 0)
	{
		cout << "Transaction number " << trans_number << std::endl;
		runTransaction(similarity, insert_query);
		insert_query.clear();
		trans_number++;
	}

	// Evaluate all the other columns
	for (int i = 2; i <= users; i++)
	{
		temp_s = "ALTER TABLE similarity ADD COLUMN user";
		temp_s.append(std::to_string(i).c_str());
		temp_s.append(" double;");
		const char *query = temp_s.c_str();
		insert_query.push_back(query);
		if (runTransaction(similarity, insert_query) == 0) {
			break;
		}
		insert_query.clear();

		for (int j = i; j <= users; j++)
		{
			sim = pearson_similarity(i, j, db);
			if (isnan(sim)) {
				sim = 10;
			}

			std::string temp_s = "UPDATE similarity SET user";
			temp_s.append(std::to_string(i).c_str());
			temp_s.append(" = ");
			temp_s.append(std::to_string(sim).c_str());
			temp_s.append(" WHERE userID = ");
			temp_s.append(std::to_string(j).c_str());
			temp_s.append(";");

			query = temp_s.c_str();
			insert_query.push_back(query);

			if ((n % interval) == 0)
			{
				cout << "Transaction number " << trans_number << std::endl;
				if (runTransaction(similarity, insert_query) == 0) {
					break;
				}
				insert_query.clear();
				trans_number++;
			}
			n++;
		}
	}
	if (insert_query.size() > 0)
	{
		cout << "Transaction number " << trans_number << std::endl;
		runTransaction(similarity, insert_query);
		insert_query.clear();
		trans_number++;
	}
}

void update_similarity_matrix(sqlite3 *db, sqlite3 *similarity, long int users, long int interval) {

	int rows = getSimilarityRows(similarity);
	int cols = getSimilarityColumns(similarity);

	cout << "Actual dimensions of similarity matrix: " << rows << "X" << cols << std::endl;

	int n = 1;
	int trans_number = 1;
	double sim; // pearson coefficient (from -1 to 1, if NaN we set it to a value of 10)
	char *zErrMsg = 0;
	std::string temp_s;
	vector<std::string> insert_query;

	if (rows == cols && rows == users) {
		cout << "The similarity matrix is already up to date." << std::endl;
	}

	if (rows < users) {

		int nextRow = rows + 1;
		//cout << "Start updating from row: " << nextRow << std::endl;

		// Add the rows 
		for (int i = nextRow; i <= users; i++) // This cycle on the row index
		{
			cout << "Row: " << i << std::endl;
			sim = pearson_similarity(i, 1, db);
			if (isnan(sim)) {
				sim = 10;
			}
			temp_s = "INSERT INTO similarity(userID,user";
			temp_s.append(std::to_string(1).c_str());
			temp_s.append(") VALUES(");
			temp_s.append(std::to_string(i).c_str());
			temp_s.append(",");
			temp_s.append(std::to_string(sim).c_str());
			temp_s.append(");");
			const char *query = temp_s.c_str();
			insert_query.push_back(query);

			for (int j = 2; j <= cols; j++) // This cycle on the column index
			{
				sim = pearson_similarity(i, j, db);
				if (isnan(sim)) {
					sim = 10;
				}

				std::string temp_s = "UPDATE similarity SET user";
				temp_s.append(std::to_string(j).c_str());
				temp_s.append(" = ");
				temp_s.append(std::to_string(sim).c_str());
				temp_s.append(" WHERE userID = ");
				temp_s.append(std::to_string(i).c_str());
				temp_s.append(";");

				query = temp_s.c_str();
				insert_query.push_back(query);

				if ((n % interval) == 0)
				{
					cout << "Transaction number " << trans_number << std::endl;
					if (runTransaction(similarity, insert_query) == 0) {
						break;
					}
					insert_query.clear();
					trans_number++;
				}
				n++;
			}
		}

		if (insert_query.size() > 0)
		{
			cout << "Transaction number " << trans_number << std::endl;
			runTransaction(similarity, insert_query);
			insert_query.clear();
			trans_number++;
		}

		int nextCol = cols + 1;
		//cout << "Starting now with column: " << nextCol << std::endl;

		// Continue with the columns
		for (int i = nextCol; i <= users; i++) {
			cout << "Column: " << i << std::endl;
			// Insert the new column
			temp_s = "ALTER TABLE similarity ADD COLUMN user";
			temp_s.append(std::to_string(i).c_str());
			temp_s.append(" double;");
			const char *query = temp_s.c_str();
			insert_query.push_back(query);
			if (runTransaction(similarity, insert_query) == 0) {
				break;
			}
			insert_query.clear();

			// Populate the new column
			for (int j = i + 1; j <= users; j++)
			{
				sim = pearson_similarity(i, j, db);
				if (isnan(sim)) {
					sim = 10;
				}

				std::string temp_s = "UPDATE similarity SET user";
				temp_s.append(std::to_string(i).c_str());
				temp_s.append(" = ");
				temp_s.append(std::to_string(sim).c_str());
				temp_s.append(" WHERE userID = ");
				temp_s.append(std::to_string(j).c_str());
				temp_s.append(";");

				query = temp_s.c_str();
				insert_query.push_back(query);

				if ((n % interval) == 0)
				{
					cout << "Transaction number " << trans_number << std::endl;
					if (runTransaction(similarity, insert_query) == 0) {
						break;
					}
					insert_query.clear();
					trans_number++;
				}
				n++;
			}
		}

		if (insert_query.size() > 0)
		{
			cout << "Transaction number " << trans_number << std::endl;
			runTransaction(similarity, insert_query);
			insert_query.clear();
			trans_number++;
		}
	}

	if (rows >= users && cols < users) {
		int nextCol = cols;
		const char *query;
		for (int i = nextCol; i <= rows; i++) {

			// Insert the new column
			cout << "Column: " << i << std::endl;
			if (i != nextCol) {
				temp_s = "ALTER TABLE similarity ADD COLUMN user";
				temp_s.append(std::to_string(i).c_str());
				temp_s.append(" double;");
				query = temp_s.c_str();
				insert_query.push_back(query);
				if (runTransaction(similarity, insert_query) == 0) {
					break;
				}
				insert_query.clear();
			}
			// Populate the new column
			for (int j = i + 1; j <= users; j++)
			{
				sim = pearson_similarity(i, j, db);
				if (isnan(sim)) {
					sim = 10;
				}

				std::string temp_s = "UPDATE similarity SET user";
				temp_s.append(std::to_string(i).c_str());
				temp_s.append(" = ");
				temp_s.append(std::to_string(sim).c_str());
				temp_s.append(" WHERE userID = ");
				temp_s.append(std::to_string(j).c_str());
				temp_s.append(";");

				query = temp_s.c_str();
				insert_query.push_back(query);

				if ((n % interval) == 0)
				{
					cout << "Transaction number " << trans_number << std::endl;
					if (runTransaction(similarity, insert_query) == 0) {
						break;
					}
					insert_query.clear();
					trans_number++;
				}
				n++;
			}
		}

		if (insert_query.size() > 0)
		{
			cout << "Transaction number " << trans_number << std::endl;
			runTransaction(similarity, insert_query);
			insert_query.clear();
			trans_number++;
		}
	}

	rows = getSimilarityRows(similarity);
	cols = getSimilarityColumns(similarity);

	cout << "Final dimensions of similarity matrix: " << rows << "X" << cols << std::endl;
}
*/

double pearson_similarity(vector<entry> user1, vector<entry> user2, double avg1, double avg2, sqlite3 *db)
{
	long int u1 = user1[0].user;
	long int u2 = user2[0].user;
	if (u1 == u2)
	{
		return 1.0;
	}

	user2.insert(user2.end(), user1.begin(), user1.end());
	std::sort(user2.begin(), user2.end(), compareItems);

	int matches = 0;
	double newnum = 0;
	double newden1 = 0;
	double newden2 = 0;

#pragma omp parallel for reduction(+:newnum), reduction(+:newden1), reduction(+:newden2)

	for (int i = 0; i < user2.size() - 1; i++)
	{
		if (user2[i].item == user2[i + 1].item)
		{
			double avg_int1;
			double avg_int2;
			if (user2[i].user == u1)
			{
				avg_int1 = avg1;
			}
			else {
				avg_int1 = avg2;
			}
			if (user2[i + 1].user == u1)
			{
				avg_int2 = avg1;
			}
			else {
				avg_int2 = avg2;
			}
			double diff1 = (user2[i].rating - avg_int1);
			double diff2 = (user2[i + 1].rating - avg_int2);
			newnum += diff1 * diff2;
			newden1 += pow(diff1, 2);
			newden2 += pow(diff2, 2);
			matches++;
		}
	}
	return newnum / (sqrt(newden1) * sqrt(newden2));
}



void similarity_matrix(sqlite3 *db, sqlite3 *db_avg, sqlite3 *similarity, long int tot_users, long int interval) {

	int check;
	int trans_number = 1;
	long int n = 1;
	double sim; // pearson coefficient (from -1 to 1, if NaN we set it to a value of 10)
	char *zErrMsg = 0;
	const char *pzTest;
	string temp_s;
	vector<string> insert_query;
	sqlite3_stmt *stmt;

	std::cout << "Evaluate all pearson coefficient related to the first user and inserting them on the DB..." << std::endl;
	string select1 = "SELECT * from usersratings WHERE usersratings.user = 1";

	vector<entry> user1 = query(select1.c_str(), db);
	map<long int, vector<entry>> users;

	string select_avg1 = "SELECT avg from averages WHERE averages.user = 1";
	double avg1 = avg_value(select_avg1.c_str(), db_avg);
	map<long int, double> avgs;

	for (long int i = 2; i <= tot_users; i++)
	{
		// Each user is saved also in a map in order to speed up the evaluation
		string select = "SELECT * from usersratings WHERE usersratings.user = " + to_string(i);
		users[i] = query(select.c_str(), db);
		string select_avg = "SELECT avg from averages WHERE averages.user = " + to_string(i);
		avgs[i] = avg_value(select_avg.c_str(), db_avg);
		sim = pearson_similarity(user1, users[i], avg1, avgs[i], db);
		if (!isnan(sim)) {
			string to_insert = "INSERT INTO similarity(user1, user2, sim) VALUES(" + to_string(1) + ", " + to_string(i) + ", " + to_string(sim) + ")";
			insert_query.push_back(to_insert);
			if ((n % interval) == 0)
			{
				std::cout << "Transaction number " << trans_number << std::endl;
				if (runTransaction(similarity, insert_query) == 0) {
					break;
				}
				insert_query.clear();
				trans_number++;
			}
			n++;
		}
	}

	// Writing on DB what's left of the first user pearson coefficients
	if (insert_query.size() > 0)
	{
		std::cout << "Transaction number " << trans_number << std::endl;
		runTransaction(similarity, insert_query);
		insert_query.clear();
		trans_number++;
	}

	std::wcout << "Starting for the rest of the users..." << std::endl;

	// Evaluate all the other columns

	for (int i = 2; i <= tot_users; i++)
	{
		std::cout << "Writing user " << i << " ..." << std::endl;
		for (int j = i+1; j < tot_users; j++)
		{
			//std::cout << "Comparison with " << j << " ..." << std::endl;
			sim = pearson_similarity(users[i], users[j], avgs[i], avgs[j], db);
			if (!isnan(sim)) {
				string to_insert = "INSERT INTO similarity(user1, user2, sim) VALUES(" + to_string(i) + ", " + to_string(j) + ", " + to_string(sim) + ")";
				insert_query.push_back(to_insert);
				if ((n % interval) == 0)
				{
					std::cout << "Transaction number " << trans_number << std::endl;
					if (runTransaction(similarity, insert_query) == 0) {
						break;
					}
					insert_query.clear();
					trans_number++;
				}
				n++;
			}
		}
		//std::cout << "Finished row " << i << " ..." << std::endl;
	}

	// Writing on DB what's left on the array of queries
	if (insert_query.size() > 0)
	{
		std::cout << "Transaction number " << trans_number << std::endl;
		runTransaction(similarity, insert_query);
		insert_query.clear();
		trans_number++;
	}
}

// --------------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------

int main(int argc, const char * argv[]) {

	int rc, rc_2, avgs;
	long int users = 135000;
	long int total_users;
	long int commit_interval = 10000;
	bool create = true; // Flag value: if create is false, we just update the similarity matrix that already exists.
	sqlite3 *db, *db_2, *db_avgs;
	sqlite3_stmt *stmt;
	const char *queryTail;

	// Open DB for user ratings
	rc = sqlite3_open("C:/sqlite/test", &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return(0);
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}

	// Open DB for similarity matrix
	rc_2 = sqlite3_open("C:/sqlite/prova", &db_2);
	if (rc_2) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_2));
		return(0);
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}

	// Open DB for averages
	avgs = sqlite3_open("C:/sqlite/avgs", &db_avgs);
	if (avgs != SQLITE_OK) {
		fprintf(stderr, "Can't open averages database: %s\n", sqlite3_errmsg(db_avgs));
		return 1;
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}

	// Get the total number of users
	char *max_users_query = "SELECT * from usersratings WHERE user=(SELECT max(user) FROM usersratings)";
	vector<entry> max = query(max_users_query, db);
	cout << "total number of users is: " << max[0].user << std::endl;
	total_users = max[0].user;

	if (create)
	{
		char *creation_query = "CREATE TABLE similarity(user1 int, user2 int, sim DOUBLE);";
		if (sqlite3_prepare_v2(db_2, creation_query, -1, &stmt, &queryTail) == SQLITE_OK) {

			sqlite3_step(stmt);
			sqlite3_finalize(stmt);

			high_resolution_clock::time_point t1 = high_resolution_clock::now();
			similarity_matrix(db, db_avgs, db_2, users, commit_interval);
			high_resolution_clock::time_point t2 = high_resolution_clock::now();
			auto duration = duration_cast<seconds>(t2 - t1).count();
			cout << "Time to create similarity matrix: " << duration << std::endl;
		}
		else {
			cout << "An error occured. Database hasn't been created." << std::endl;
		}
	}



	/*std::vector<entry> u;
	//Execute SQL statement
	//rc = sqlite3_exec(db, sql, callback, &u, &zErrMsg);

	if (rc != SQLITE_OK) {
	fprintf(stderr, "SQL error: %s\n", zErrMsg);
	sqlite3_free(zErrMsg);
	}
	else {
	fprintf(stdout, "Operation done successfully\n");
	}
	*/

	sqlite3_close(db);
	sqlite3_close(db_2);
	return 0;
}

