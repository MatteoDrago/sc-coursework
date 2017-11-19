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

bool compareItems(const entry &a, const entry &b)
{
	return a.item < b.item || a.item == b.item && a.user < b.user;
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
			if (user2[i+1].user == u1)
			{
				avg_int2 = avg1;
			}
			else {
				avg_int2 = avg2;
			}
			double diff1 = (user2[i].rating - avg_int1);
			double diff2 = (user2[i+1].rating - avg_int2);
			num += diff1 * diff2;
			den1 += pow(diff1, 2);
			den2 += pow(diff2, 2);
			matches++;
		}
	}
	
	//cout << "Pearson coefficient: " << num / (sqrt(den1) * sqrt(den2)) << std::endl;
	return num / (sqrt(den1) * sqrt(den2));
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

void similarity_matrix(sqlite3 *db, sqlite3 *similarity, long int users, long int interval) {

	int check;
	int trans_number = 1;
	long int n = 1;
	double sim; // pearson coefficient (from -1 to 1, if NaN we set it to a value of 10)
	char *zErrMsg = 0;
	const char *pzTest;
	std::string temp_s;
	vector<std::string> insert_query;
	sqlite3_stmt *stmt;

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

int main(int argc, const char * argv[]) {

	int rc, rc_2, check;
	long int users = 50;
	long int commit_interval = 1000;
	vector<double> pearson_u1;
	sqlite3 *db, *db_2;

	// Just few instructions to make sure that the two databases are ready to be managed
	rc = sqlite3_open("C:/sqlite/test", &db);

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return(0);
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}

	rc_2 = sqlite3_open("D:/Social Computing Techniques/prova", &db_2);

	if (rc_2) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_2));
		return(0);
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}

	// Get the total number of users
	char *max_users_query = "SELECT * from usersratings WHERE user=(SELECT max(user) FROM usersratings)";
	vector<entry> max = query(max_users_query, db);
	cout << "total number of users is: " << max[0].user << std::endl;
	users = max[0].user;

	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	similarity_matrix(db, db_2, users, commit_interval);
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	auto duration = duration_cast<seconds>(t2 - t1).count();
	cout << "Time to evaluate similarity matrix: " << duration << std::endl;
	

	// database index 

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
    return 0;
}

