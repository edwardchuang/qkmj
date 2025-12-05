/*
 * Server  test
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "mjgps.h"
#include "mongo.h"

struct player_record record;

// Helper prototypes (from mjgps_mongo_helpers.c)
void bson_to_record(const bson_t *doc, struct player_record *rec);
bson_t *record_to_bson(const struct player_record *rec);

int
read_user_name (const char *name)
{
    bson_t *query;
    bson_t *doc;

    query = BCON_NEW("name", BCON_UTF8(name));
    doc = mongo_find_document("qkmj", "users", query);
    bson_destroy(query);

    if (doc) {
        bson_to_record(doc, &record);
        bson_destroy(doc);
        return 1;
    }
    return 0;
}

int
read_user_id (unsigned int id)
{
    bson_t *query;
    bson_t *doc;

    query = BCON_NEW("user_id", BCON_INT64((int64_t)id));
    doc = mongo_find_document("qkmj", "users", query);
    bson_destroy(query);

    if (doc) {
        bson_to_record(doc, &record);
        bson_destroy(doc);
        return 1;
    }
    return 0;
}

void
write_record ()
{
    bson_t *query;
    bson_t *update;
    bson_t *doc;

    query = BCON_NEW("user_id", BCON_INT64((int64_t)record.id));
    doc = record_to_bson(&record);
    update = BCON_NEW("$set", BCON_DOCUMENT(doc));
    
    if (!mongo_update_document("qkmj", "users", query, update)) {
        printf("Update failed for user_id %d\n", record.id);
    } else {
        printf("User record updated.\n");
    }

    bson_destroy(query);
    bson_destroy(update);
    bson_destroy(doc);
}

void
delete_user_by_id(unsigned int id) {
    bson_t *query;
    
    query = BCON_NEW("user_id", BCON_INT64((int64_t)id));
    if (mongo_delete_document("qkmj", "users", query)) {
        printf("User %d deleted.\n", id);
    } else {
        printf("Failed to delete user %d.\n", id);
    }
    bson_destroy(query);
}

void
print_record_row(struct player_record *tmprec) {
    char time1[40], time2[40];
    
    // Format times
    char *t1 = ctime((time_t*)&tmprec->regist_time);
    char *t2 = ctime((time_t*)&tmprec->last_login_time);
    
    if (t1) {
        strcpy(time1, t1);
        time1[strlen(time1) - 1] = 0;
    } else {
        strcpy(time1, "Unknown");
    }

    if (t2) {
        strcpy(time2, t2);
        time2[strlen(time2) - 1] = 0;
    } else {
        strcpy(time2, "Unknown");
    }

    printf ("%d %10s %15s %ld %d %d %d  %s\n", tmprec->id, tmprec->name,
        tmprec->password, tmprec->money, tmprec->level, tmprec->login_count,
        tmprec->game_count, tmprec->last_login_from);

    printf ("              %s    %s\n", time1, time2);
}

void
print_record ()
{
    struct player_record tmprec;
    int player_num = 0;
    int i;
    int id;
    char name[40];
    long money;
    bson_t *query = NULL;
    mongoc_cursor_t *cursor;
    const bson_t *doc;

    printf ("(1) 以 id 查看特定使用者\n");
    printf ("(2) 以名稱查看特定使用者\n");
    printf ("(3) 查看所有使用者\n");
    printf ("(4) 查看此金額以上的使用者\n");
    printf ("(5) 查看此金額以下的使用者\n");
    printf ("\n請輸入你的選擇:");
    if (scanf ("%d", &i) != 1) return;

    switch (i)
    {
    case 1:
        printf ("請輸入你要查看的 id:");
        if (scanf ("%d", &id) != 1) return;
        if (id < 0) return;
        query = BCON_NEW("user_id", BCON_INT64((int64_t)id));
        break;

    case 2:
        printf ("請輸入你要查看的名稱:");
        char c;
        while((c = getchar()) != '\n' && c != EOF); // flush stdin
        if (fgets(name, sizeof(name), stdin)) {
            name[strcspn(name, "\n")] = 0;
        }
        query = BCON_NEW("name", BCON_UTF8(name));
        break;

    case 3:
        query = bson_new(); // Empty query
        break;

    case 4:
        printf ("請輸入金額:");
        if (scanf ("%ld", &money) != 1) return;
        query = BCON_NEW("money", "{", "$gte", BCON_INT64((int64_t)money), "}");
        break;
    case 5:
        printf ("請輸入金額:");
        if (scanf ("%ld", &money) != 1) return;
        query = BCON_NEW("money", "{", "$lte", BCON_INT64((int64_t)money), "}");
        break;

    default:
        return;
    }

    cursor = mongo_find_documents("qkmj", "users", query);
    bson_destroy(query);

    if (!cursor) {
        printf("Query failed.\n");
        return;
    }

    while (mongoc_cursor_next(cursor, &doc)) {
        bson_to_record(doc, &tmprec);
        print_record_row(&tmprec);
        player_num++;
    }
    mongoc_cursor_destroy(cursor);

    printf ("--------------------------------------------------------------\n");
    if (i == 3 || i == 4 || i == 5)
        printf ("共 %d 人符合條件\n", player_num);
}

void
modify_user ()
{
    int i;
    char name[40];
    char account[40];
    long money;

    printf ("請輸入使用者帳號:");
    char c;
    while((c = getchar()) != '\n' && c != EOF); // flush stdin
    if (fgets(account, sizeof(account), stdin)) {
        account[strcspn(account, "\n")] = 0;
    }
    
    if(!read_user_name(account)){
        printf(" 查無此人 ");
        return;
    }
    
    printf ("\n");
    printf ("(1) 更改名稱\n");
    printf ("(2) 重設密碼\n");
    printf ("(3) 更改金額\n");
    printf ("(4) 取消更改\n");
    printf ("\n請輸入你的選擇:");
    if (scanf ("%d", &i) != 1) return;
    printf ("\n");

    switch (i)
    {
    case 1:
        printf ("請輸入要更改的名稱:");
        while((c = getchar()) != '\n' && c != EOF); // flush stdin
        if (fgets(name, sizeof(name), stdin)) {
            name[strcspn(name, "\n")] = 0;
        }
        strcpy (record.name, name);
        printf ("改名為 %s\n", name);
        break;

    case 2:
        record.password[0] = 0; // Or set to a default? Original code set to empty.
        printf ("密碼已重設!\n");
        break;

    case 3:
        printf ("請輸入要更改的金額:");
        if (scanf ("%ld", &money) != 1) return;
        record.money = money;
        printf ("金額更改為 %ld\n", money);
        break;

    default:
        return;
    }
    write_record ();
}

// --- Unit Tests ---

void run_tests() {
    printf("Running Unit Tests...\n");

    // 1. Clean up test user if exists
    const char *test_name = "UnitTestUser";
    bson_t *del_query = BCON_NEW("name", BCON_UTF8(test_name));
    mongo_delete_document("qkmj", "users", del_query);
    bson_destroy(del_query);

    // 2. Create User (Simulate add_user logic from mjgps)
    printf("[TEST] Creating user '%s'...\n", test_name);
    struct player_record new_rec;
    memset(&new_rec, 0, sizeof(new_rec));
    int64_t nid = mongo_get_next_sequence("qkmj", "userid");
    new_rec.id = (unsigned int)nid;
    strcpy(new_rec.name, test_name);
    strcpy(new_rec.password, "testpass");
    new_rec.money = 1000;
    new_rec.regist_time = time(NULL);

    bson_t *doc = record_to_bson(&new_rec);
    bool inserted = mongo_insert_document("qkmj", "users", doc);
    bson_destroy(doc);
    
    if (inserted) printf("[PASS] Insert user success.\n");
    else { printf("[FAIL] Insert user failed.\n"); return; }

    // 3. Read User by Name
    printf("[TEST] Reading user by name...\n");
    if (read_user_name(test_name)) {
        printf("[PASS] Found user. ID: %d, Money: %ld\n", record.id, record.money);
        assert(record.id == new_rec.id);
        assert(record.money == 1000);
    } else {
        printf("[FAIL] User not found.\n");
        return;
    }

    // 4. Update User
    printf("[TEST] Updating user money...\n");
    record.money = 5000;
    write_record(); // Updates global 'record' to DB

    // Verify Update
    if (read_user_id(record.id)) {
        if (record.money == 5000) printf("[PASS] Update verified.\n");
        else printf("[FAIL] Update failed. Money is %ld\n", record.money);
    }

    // 5. Delete User
    printf("[TEST] Deleting user...\n");
    delete_user_by_id(record.id);
    if (!read_user_id(record.id)) {
        printf("[PASS] Delete verified (User not found).\n");
    } else {
        printf("[FAIL] User still exists.\n");
    }

    printf("All tests completed.\n");
}

int
main (int argc, char **argv)
{
    int i, id;

    mongo_connect("mongodb://localhost:27017");

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        run_tests();
        mongo_disconnect();
        return 0;
    }

    while (1)
    {
        printf ("\n");
        printf ("(1) 列出所有使用者資料\n");
        printf ("(2) 刪除使用者\n");
        printf ("(3) 更改使用者資料\n");
        printf ("(4) 離開\n\n");
        printf ("請輸入你的選擇:");
        if (scanf ("%d", &i) != 1) break;

        switch (i)
        {
        case 1:
            print_record ();
            break;

        case 2:
            printf ("請輸入使用者代號:");
            if (scanf ("%d", &id) == 1 && id >= 0)
            {
                // Confirm? Nah, legacy didn't confirm.
                delete_user_by_id(id);
            }
            break;

        case 3:
            modify_user ();
            break;

        case 4:
default:
            mongo_disconnect();
            return 0;
        }
    }

    mongo_disconnect();
    return 0;
}