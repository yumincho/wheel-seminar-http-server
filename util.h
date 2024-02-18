#include <pthread.h>

#define SESSION_MAXAGE 60  // 1분

/**
 * 텍스트 파일을 통째로 읽어옵니다. 성공하면 읽은 바이트 수를, 실패하면 음수를
 * 반환합니다.
 *
 * 예제:
 * char* content;
 * ReadTextFile("test.txt", &content);
 */
int ReadTextFile(const char* path, char** result);
/**
 * 문자열을 특정 구분자를 기준으로 나눕니다. strtok과 달리 원본 문자열을
 * 손상시키지 않습니다. 성공하면 부분 문자열의 개수를, 실패하면 음수를
 * 반환합니다.
 *
 * 예제:
 * char** splited;
 * int count = SplitString("Hello, world!", ", ", &splited);
 */
int SplitString(const char* str, const char* delimiter, char*** results);

typedef struct {
  const char* Category;
  const char* Key;
  void* Value;
} DatabaseEntry;

typedef struct DatabaseEntryNode {
  DatabaseEntry Entry;
  struct DatabaseEntryNode* Previous;
  struct DatabaseEntryNode* Next;
} DatabaseEntryNode;

typedef struct {
  DatabaseEntryNode* First;
  DatabaseEntryNode* Last;
  pthread_mutex_t Mutex;
} Database;

int CreateDatabase(Database* db);
/**
 * 데이터베이스에 새로운 항목을 추가합니다. 성공하면 0을, 실패하면 음수를
 * 반환합니다.
 *
 * 예제:
 * AddDatabaseEntry(&db, "user", "static", "KAIST 23");
 * AddDatabaseEntry(&db, "user", "roul", "KAIST 23");
 */
int AddDatabaseEntry(Database* db, const char* category, const char* key,
                     void* value);
/**
 * 데이터베이스에서 특정 항목을 제거합니다. 성공하면 0을, 실패하면 음수를
 * 반환합니다.
 *
 * 예제:
 * RemoveDatabaseEntry(&db, "user", "static");
 */
int RemoveDatabaseEntry(Database* db, const char* category, const char* key);
/**
 * 데이터베이스에서 특정 항목을 찾습니다. 성공하면 해당 항목의 값을, 실패하면
 * NULL을 반환합니다.
 *
 * 예제:
 * void** value = FindDatabaseEntry(&db, "user", "static");
 * printf("static is %s\n", (char*)*value);
 * *value = "24 Taxi PM";
 */
void** FindDatabaseEntry(Database* db, const char* category, const char* key);

/**
 * 회원가입을 합니다. 성공하면 0을, 실패하면 음수를 반환합니다.
 */
int Register(Database* db, const char* id, const char* password);
/**
 * 로그인을 해 새로운 세션을 만듭니다. 성공하면 0을, 실패하면 음수를 반환합니다.
 * 성공하면 *sessionID에 새로운 새션 ID가 저장됩니다.
 */
int Login(Database* db, const char* id, const char* password, char** sessionID);
/**
 * 세션 ID가 유효한지 확인합니다. 유효하면 0을, 유효하지 않으면 음수를
 * 반환합니다.
 */
int CheckSession(Database* db, const char* sessionID);

/**
 * 사용자가 저장한 텍스트를 가져옵니다. 성공하면 텍스트를, 실패하면 NULL을
 * 반환합니다.
 */
const char* GetUserText(Database* db, const char* sessionID);
/**
 * 텍스트를 저장합니다. 성공하면 0을, 실패하면 음수를 반환합니다.
 */
int SetUserText(Database* db, const char* sessionID, const char* text);