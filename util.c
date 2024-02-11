#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ReadTextFile(const char* path, char** result) {
	FILE* const file = fopen(path, "r");
	if (file == NULL) {
		return -1;
	}

	fseek(file, 0, SEEK_END);
	const long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* content = (char*)malloc(size + 1);
	if (content == NULL) {
		fclose(file);
		return -1;
	} else {
		fread(content, 1, size, file);
		content[size] = '\0';
	}

	*result = content;

	fclose(file);
	return size;
}
int SplitString(const char* str, const char* delimiter, char*** results) {
	const int delimiterLength = strlen(delimiter);

	char** splited = NULL;
	int count = 0;
	char* last;
	while ((last = strstr(str, delimiter)) != NULL) {
		const int length = last - str;
		if (length > 0) {
			splited = (char**)realloc(splited, sizeof(char*) * (count + 1));
			if (splited == NULL) {
				return -1;
			}

			splited[count] = (char*)malloc(length + 1);
			if (splited[count] == NULL) {
				return -1;
			}

			strncpy(splited[count], str, length);
			splited[count][length] = '\0';
			count++;
		}

		str = last + delimiterLength;
	}

	const int length = strlen(str);
	if (length > 0) {
		splited = (char**)realloc(splited, sizeof(char*) * (count + 1));
		if (splited == NULL) {
			return -1;
		}

		splited[count] = (char*)malloc(length + 1);
		if (splited[count] == NULL) {
			return -1;
		}

		strcpy(splited[count], str);
		count++;
	}

	*results = splited;
	return count;
}

int CreateDatabase(Database* db) {
	db->First = NULL;
	db->Last = NULL;
	pthread_mutex_init(&db->Mutex, NULL);

	return 0;
}
int AddDatabaseEntry(Database* db, const char* category, const char* key, void* value) {
	if (FindDatabaseEntry(db, category, key) != NULL) {
		return -1;
	}

	pthread_mutex_lock(&db->Mutex);

	DatabaseEntryNode* node = (DatabaseEntryNode*)malloc(sizeof(DatabaseEntryNode));
	if (node == NULL) {
		pthread_mutex_unlock(&db->Mutex);
		return -1;
	} else {
		node->Entry.Category = category;
		node->Entry.Key = key;
		node->Entry.Value = value;
		node->Previous = db->Last;
		node->Next = NULL;
	}

	if (db->First == NULL) {
		db->First = node;
	} else {
		db->Last->Next = node;
	}
	db->Last = node;

	pthread_mutex_unlock(&db->Mutex);

	return 0;
}
int RemoveDatabaseEntry(Database* db, const char* category, const char* key) {
	pthread_mutex_lock(&db->Mutex);

	DatabaseEntryNode* node = db->First;
	while (node != NULL) {
		if (strcmp(node->Entry.Category, category) == 0 &&
			strcmp(node->Entry.Key, key) == 0) {

			if (node->Previous != NULL) {
				node->Previous->Next = node->Next;
			} else {
				db->First = node->Next;
			}
			if (node->Next != NULL) {
				node->Next->Previous = node->Previous;
			} else {
				db->Last = node->Previous;
			}

			free(node);
			pthread_mutex_unlock(&db->Mutex);

			return 0;
		}

		node = node->Next;
	}

	pthread_mutex_unlock(&db->Mutex);

	return -1;
}
void** FindDatabaseEntry(Database* db, const char* category, const char* key) {
	pthread_mutex_lock(&db->Mutex);

	DatabaseEntryNode* node = db->First;
	while (node != NULL) {
		if (strcmp(node->Entry.Category, category) == 0 &&
			strcmp(node->Entry.Key, key) == 0) {

			pthread_mutex_unlock(&db->Mutex);

			return &node->Entry.Value;
		}

		node = node->Next;
	}

	pthread_mutex_unlock(&db->Mutex);

	return NULL;
}