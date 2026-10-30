#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "overscanf.h"

static int failed = 0;

static void expect_int(const char *title, int count, int got, int expected_count, int expected)
{
    if (count != expected_count || got != expected) {
        printf("FAIL %s: count=%d value=%d, ожидалось count=%d value=%d\n",
               title, count, got, expected_count, expected);
        failed++;
        return;
    }
    printf("%s -> %d\n", title, got);
}

static void expect_uint(const char *title, int count, unsigned got, int expected_count, unsigned expected)
{
    if (count != expected_count || got != expected) {
        printf("FAIL %s: count=%d value=%u, ожидалось count=%d value=%u\n",
               title, count, got, expected_count, expected);
        failed++;
        return;
    }
    printf("%s -> %u\n", title, got);
}

int main(void)
{
    int value = 0;
    int second = 0;
    unsigned zeck = 0;
    int count;
    char word[32];
    char set[32];
    char letters[8];
    double real = 0.0;
    int marker = -1;
    FILE *in;

    count = oversscanf("MCMXCIV", "%Ro", &value);
    expect_int("римское 1994", count, value, 1, 1994);
    count = oversscanf("IV", "%Ro", &value);
    expect_int("римское 4", count, value, 1, 4);
    count = oversscanf("IX", "%Ro", &value);
    expect_int("римское 9", count, value, 1, 9);
    count = oversscanf("LVIII", "%Ro", &value);
    expect_int("римское 58", count, value, 1, 58);

    count = oversscanf("1", "%Zr", &zeck);
    expect_uint("цекендорф 0", count, zeck, 1, 0);
    count = oversscanf("11", "%Zr", &zeck);
    expect_uint("цекендорф 1", count, zeck, 1, 1);
    count = oversscanf("011", "%Zr", &zeck);
    expect_uint("цекендорф 2", count, zeck, 1, 2);
    count = oversscanf("0011", "%Zr", &zeck);
    expect_uint("цекендорф 3", count, zeck, 1, 3);
    count = oversscanf("1011", "%Zr", &zeck);
    expect_uint("цекендорф 4", count, zeck, 1, 4);
    count = oversscanf("010011", "%Zr", &zeck);
    expect_uint("цекендорф 10", count, zeck, 1, 10);

    count = oversscanf("ff", "%Cv", &value, 16);
    expect_int("ff основание 16", count, value, 1, 255);
    count = oversscanf("FF", "%CV", &value, 16);
    expect_int("FF основание 16", count, value, 1, 255);
    count = oversscanf("-a", "%Cv", &value, 16);
    expect_int("-a основание 16", count, value, 1, -10);
    count = oversscanf("1010", "%Cv", &value, 2);
    expect_int("1010 основание 2", count, value, 1, 10);
    count = oversscanf("10", "%Cv", &value, 99);
    expect_int("основание вне диапазона", count, value, 1, 10);
    count = oversscanf("FF", "%Cv", &value, 16);
    if (count != 0) {
        printf("FAIL верхний регистр для %%Cv не отклонён, count=%d value=%d\n", count, value);
        failed++;
    } else {
        printf("верхний регистр для %%Cv отклонён\n");
    }

    count = oversscanf("MCMXCIV 1011 ff", "%Ro %Zr %Cv", &value, &zeck, &second, 16);
    if (count != 3 || value != 1994 || zeck != 4 || second != 255) {
        printf("FAIL смесь: count=%d roman=%d zeck=%u base=%d\n", count, value, zeck, second);
        failed++;
    } else {
        printf("смесь -> 1994, 4, 255\n");
    }

    count = oversscanf("42 -7", "%d %d", &value, &second);
    if (count != 2 || value != 42 || second != -7) {
        printf("FAIL %%d: %d %d count=%d\n", value, second, count);
        failed++;
    } else {
        printf("%%d -> 42 -7\n");
    }
    count = oversscanf("42 7", "%*d %d", &value);
    expect_int("подавление присваивания", count, value, 1, 7);
    count = oversscanf("0x10 010", "%i %i", &value, &second);
    if (count != 2 || value != 16 || second != 8) {
        printf("FAIL %%i: %d %d count=%d\n", value, second, count);
        failed++;
    } else {
        printf("%%i -> 16 8\n");
    }
    count = oversscanf("ff 377", "%x %o", &zeck, &value);
    if (count != 2 || zeck != 255 || value != 255) {
        printf("FAIL %%x/%%o: %u %d count=%d\n", zeck, value, count);
        failed++;
    } else {
        printf("%%x и %%o -> 255 255\n");
    }
    count = oversscanf("-2147483648", "%d", &value);
    expect_int("INT_MIN", count, value, 1, INT_MIN);

    count = oversscanf("hello world", "%s", word);
    if (count != 1 || strcmp(word, "hello") != 0) {
        printf("FAIL %%s: [%s] count=%d\n", word, count);
        failed++;
    } else {
        printf("%%s -> hello\n");
    }
    count = oversscanf("abc", "%[a-c]", set);
    if (count != 1 || strcmp(set, "abc") != 0) {
        printf("FAIL набор: [%s]\n", set);
        failed++;
    } else {
        printf("набор -> abc\n");
    }
    count = oversscanf("Z", "%c", letters);
    if (count != 1 || letters[0] != 'Z') {
        printf("FAIL %%c\n");
        failed++;
    } else {
        printf("%%c -> Z\n");
    }
    count = oversscanf("1.5 -2.25", "%lf %lf", &real, &real);
    if (count != 2 || real != -2.25) {
        printf("FAIL вещественное: %g count=%d\n", real, count);
        failed++;
    } else {
        printf("вещественное -> -2.25\n");
    }
    count = oversscanf("abX", "ab%n%c", &marker, letters);
    if (count != 1 || marker != 2 || letters[0] != 'X') {
        printf("FAIL %%n: marker=%d char=%d count=%d\n", marker, letters[0], count);
        failed++;
    } else {
        printf("%%n -> 2\n");
    }
    count = oversscanf("abc", "%d", &value);
    if (count != 0) {
        printf("FAIL нечисловая строка для %%d, count=%d\n", count);
        failed++;
    } else {
        printf("нечисловая строка для %%d отклонена\n");
    }
    count = oversscanf("", "%d", &value);
    if (count != EOF) {
        printf("FAIL пустой ввод, count=%d\n", count);
        failed++;
    } else {
        printf("пустой ввод -> EOF\n");
    }
    count = oversscanf("1", NULL, &value);
    if (count != EOF) {
        printf("FAIL NULL формат\n");
        failed++;
    } else {
        printf("NULL формат отклонён\n");
    }

    in = fopen("task6_in.txt", "w+");
    if (in == NULL) {
        printf("FAIL не удалось создать файл\n");
        failed++;
    } else {
        fputs("XL 1011", in);
        rewind(in);
        value = 0;
        zeck = 0;
        count = overfscanf(in, "%Ro %Zr", &value, &zeck);
        if (count != 2 || value != 40 || zeck != 4) {
            printf("FAIL файл: count=%d roman=%d zeck=%u\n", count, value, zeck);
            failed++;
        } else {
            printf("overfscanf -> 40, 4\n");
        }
        fclose(in);
        remove("task6_in.txt");
    }

    if (failed != 0) {
        printf("Не пройдено проверок: %d\n", failed);
        return 1;
    }
    printf("Все проверки задания 6 пройдены.\n");
    return 0;
}
