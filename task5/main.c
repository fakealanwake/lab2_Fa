#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "overprintf.h"

static int failed = 0;

static void expect(const char *title, int count, const char *got, const char *expected)
{
    if (count < 0 || got == NULL || strcmp(got, expected) != 0 || count != (int)strlen(expected)) {
        printf("FAIL %s\n  получено: [%s] (%d)\n  ожидалось: [%s]\n",
               title, got == NULL ? "(null)" : got, count, expected);
        failed++;
        return;
    }
    printf("%s -> %s\n", title, got);
}

static void expect_fail(const char *title, int count)
{
    if (count >= 0) {
        printf("FAIL %s: ошибка не возвращена (%d)\n", title, count);
        failed++;
        return;
    }
    printf("%s -> отклонено\n", title);
}

int main(void)
{
    char buf[512];
    char file_buf[512];
    int count;
    int marker = -1;
    int32_t one = 1;
    int32_t zero = 0;
    float one_f = 1.0f;
    double one_d = 1.0;
    FILE *out;

    count = oversprintf(buf, "%Ro", 1994);
    expect("1994 римскими", count, buf, "MCMXCIV");
    count = oversprintf(buf, "%Ro", 4);
    expect("4 римскими", count, buf, "IV");
    count = oversprintf(buf, "%Ro", 9);
    expect("9 римскими", count, buf, "IX");
    count = oversprintf(buf, "%Ro", 58);
    expect("58 римскими", count, buf, "LVIII");
    count = oversprintf(buf, "%Ro", 0);
    expect("0 римскими", count, buf, "0");
    count = oversprintf(buf, "%Ro", -4);
    expect("-4 римскими", count, buf, "-IV");

    count = oversprintf(buf, "%Zr", 0u);
    expect("цекендорф 0", count, buf, "1");
    count = oversprintf(buf, "%Zr", 1u);
    expect("цекендорф 1", count, buf, "11");
    count = oversprintf(buf, "%Zr", 2u);
    expect("цекендорф 2", count, buf, "011");
    count = oversprintf(buf, "%Zr", 3u);
    expect("цекендорф 3", count, buf, "0011");
    count = oversprintf(buf, "%Zr", 4u);
    expect("цекендорф 4", count, buf, "1011");
    count = oversprintf(buf, "%Zr", 10u);
    expect("цекендорф 10", count, buf, "010011");

    count = oversprintf(buf, "%Cv", 255, 16);
    expect("255 основание 16", count, buf, "ff");
    count = oversprintf(buf, "%CV", 255, 16);
    expect("255 основание 16 верхний регистр", count, buf, "FF");
    count = oversprintf(buf, "%Cv", 10, 2);
    expect("10 основание 2", count, buf, "1010");
    count = oversprintf(buf, "%Cv", -10, 16);
    expect("-10 основание 16", count, buf, "-a");
    count = oversprintf(buf, "%Cv", 10, 1);
    expect("основание 1 заменено на 10", count, buf, "10");
    count = oversprintf(buf, "%Cv", 0, 16);
    expect("ноль в основании 16", count, buf, "0");

    count = oversprintf(buf, "%to", "ff", 16);
    expect("ff из 16", count, buf, "255");
    count = oversprintf(buf, "%TO", "FF", 16);
    expect("FF из 16", count, buf, "255");
    count = oversprintf(buf, "%to", "1010", 2);
    expect("1010 из 2", count, buf, "10");
    count = oversprintf(buf, "%to", "-ff", 16);
    expect("-ff из 16", count, buf, "-255");
    count = oversprintf(buf, "%to", "000", 10);
    expect("ведущие нули", count, buf, "0");
    count = oversprintf(buf, "%to", "10000000000000000", 16);
    expect("2^64", count, buf, "18446744073709551616");

    count = oversprintf(buf, "%mi", zero);
    expect("дамп нуля", count, buf, "00000000 00000000 00000000 00000000");
    count = oversprintf(buf, "%mi", one);
    expect("дамп единицы", count, buf, "00000001 00000000 00000000 00000000");
    count = oversprintf(buf, "%mu", 1u);
    expect("дамп беззнаковой единицы", count, buf, "00000001 00000000 00000000 00000000");
    count = oversprintf(buf, "%mf", one_f);
    expect("дамп float 1", count, buf,
           "00000000 00000000 10000000 00111111");
    count = oversprintf(buf, "%md", one_d);
    expect("дамп double 1", count, buf,
           "00000000 00000000 00000000 00000000 00000000 00000000 11110000 00111111");

    count = oversprintf(buf, "n=%05d r=%Ro z=%Zr b=%Cv s=%s%%", 7, 9, 4u, 255, 16, "ok");
    expect("смешанный формат", count, buf, "n=00007 r=IX z=1011 b=ff s=ok%");
    count = oversprintf(buf, "%.2f %c", 1.5, 'A');
    expect("вещественное и символ", count, buf, "1.50 A");
    count = oversprintf(buf, "ab%ncd", &marker);
    expect("%n", count, buf, "abcd");
    if (marker != 2) {
        printf("FAIL %%n записал %d, ожидалось 2\n", marker);
        failed++;
    } else {
        printf("%%n записал 2\n");
    }

    count = oversprintf(buf, "%to", "FF", 16);
    expect_fail("верхний регистр для %to", count);
    count = oversprintf(buf, "%TO", "ff", 16);
    expect_fail("нижний регистр для %TO", count);
    count = oversprintf(buf, "%q", 1);
    expect_fail("неизвестный флаг", count);
    count = oversprintf(NULL, "%d", 1);
    expect_fail("NULL буфер", count);

    out = fopen("task5_out.txt", "w+");
    if (out == NULL) {
        printf("FAIL не удалось создать временный файл\n");
        failed++;
    } else {
        count = overfprintf(out, "%Ro %Cv", 40, 255, 16);
        if (count < 0) {
            printf("FAIL overfprintf\n");
            failed++;
        } else {
            rewind(out);
            if (fgets(file_buf, sizeof(file_buf), out) == NULL || strcmp(file_buf, "XL ff") != 0) {
                printf("FAIL файл: [%s]\n", file_buf);
                failed++;
            } else {
                printf("overfprintf -> %s\n", file_buf);
            }
        }
        fclose(out);
        remove("task5_out.txt");
    }

    if (failed != 0) {
        printf("Не пройдено проверок: %d\n", failed);
        return 1;
    }
    printf("Все проверки задания 5 пройдены.\n");
    return 0;
}
