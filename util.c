#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char* CopyString(const char* str) {
	const int length = strlen(str);
	char* const result = (char*)malloc(length + 1);
	if (result == NULL) {
		return NULL;
	}

	strcpy(result, str);
	return result;
}

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
		node->Entry.Category = CopyString(category);
		node->Entry.Key = CopyString(key);
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

typedef struct {
	char* UserID; // 사용자의 아이디입니다.
	time_t CreateTime; // 세션이 생성된 시각입니다.
} Session;

int Register(Database* db, const char* id, const char* password) {
	if (FindDatabaseEntry(db, "user", id) != NULL) {
		return -1; // 이미 존재하는 아이디입니다.
	}

	AddDatabaseEntry(db, "user", id, (void*)password);

	return 0;
}
int Login(Database* db, const char* id, const char* password, char** sessionID) {
	void** const entry = *FindDatabaseEntry(db, "user", id);
	if (entry == NULL) {
		return -1; // 존재하지 않는 아이디입니다.
	}

	if (strcmp((const char*)*entry, password) != 0) {
		return -2; // 비밀번호가 틀렸습니다.
	}

	Session* const newSession = (Session*)malloc(sizeof(Session));
	if (newSession == NULL) {
		return -3;
	} else {
		newSession->UserID = CopyString(id);
	}

	char* sessionIDBuffer = (char*)calloc(32, 1);
	while (1) {
		newSession->CreateTime = time(NULL);
		sprintf(sessionIDBuffer, "%ld", newSession->CreateTime); // 생성 시각을 세션 키로 사용합니다.

		if (FindDatabaseEntry(db, "session", sessionIDBuffer) == NULL) {
			break;
		} else {
			sleep(1);
		}
	}

	AddDatabaseEntry(db, "session", sessionIDBuffer, newSession);

	*sessionID = sessionIDBuffer;
	return 0;
}
int CheckSession(Database* db, const char* sessionID) {
	void** const entry = FindDatabaseEntry(db, "session", sessionID);
	if (entry == NULL) {
		return -1;
	}

	if (time(NULL) - ((Session*)*entry)->CreateTime > 1 * 60) { // 세션의 유효 시간은 1분입니다.
		return -2;
	}

	return 0;
}

char* GetUserText(Database* db, const char* id) {
	void** const entry = FindDatabaseEntry(db, "text", id);
	return entry ? (*entry ? (char*)*entry : "") : NULL;
}
int SetUserText(Database* db, const char* id, const char* text) {
	return AddDatabaseEntry(db, "text", id, CopyString(text));
}