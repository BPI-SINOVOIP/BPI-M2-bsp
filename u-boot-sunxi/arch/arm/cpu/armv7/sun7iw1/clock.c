/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/ccmu.h>
#include <asm/arch/cpu.h>

#define  CLK_DIV(cpu_div, axi_div, ahb_div, apb_div)       \
                (((axi_div-1)<<0)|((ahb_div-1)<<4)|((apb_div-1)<<8))

static __u32 volt_freq_table[][2] =
{
	{960, 1400},
	{864, 1300},
	{720, 1200},
	{528, 1100},
	{312, 1000},
	{146,  900},
    {0,   1000}
};
struct core_pll_freq_tbl {
    __u8    FactorN;
    __u8    FactorK;
    __u8    FactorM;
    __u8    FactorP;
    __u32   Pll;
    __u32   clk_div;
};

/* core pll parameter table */
static struct core_pll_freq_tbl    CorePllTbl[] = {

    { 10,    0,    0,    3,    30,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 5  ), index = 0   */
    { 10,    0,    0,    3,    30,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 5  ), index = 1   */
    { 10,    0,    0,    3,    30,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 5  ), index = 2   */
    { 10,    0,    0,    3,    30,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 5  ), index = 3   */
    { 10,    0,    0,    3,    30,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 5  ), index = 4   */
    { 10,    0,    0,    3,    30,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 5  ), index = 5   */
    { 12,    0,    0,    3,    36,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 6  ), index = 6   */
    { 7 ,    1,    0,    3,    42,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 7  ), index = 7   */
    { 16,    0,    0,    3,    48,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 8  ), index = 8   */
    { 9 ,    1,    0,    3,    54,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 9  ), index = 9   */
    { 10,    0,    0,    2,    60,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 10 ), index = 10  */
    { 11,    0,    0,    2,    66,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 11 ), index = 11  */
    { 12,    0,    0,    2,    72,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 12 ), index = 12  */
    { 13,    0,    0,    2,    78,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 13 ), index = 13  */
    { 14,    0,    0,    2,    84,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 14 ), index = 14  */
    { 15,    0,    0,    2,    90,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 15 ), index = 15  */
    { 16,    0,    0,    2,    96,   CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 16 ), index = 16  */
    { 17,    0,    0,    2,    102,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 17 ), index = 17  */
    { 18,    0,    0,    2,    108,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 18 ), index = 18  */
    { 19,    0,    0,    2,    114,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 19 ), index = 19  */
    {  5,    0,    0,    0,    120,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 20 ), index = 20  */
    { 21,    0,    0,    2,    126,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 21 ), index = 21  */
    { 11,    0,    0,    1,    132,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 22 ), index = 22  */
    { 23,    0,    0,    2,    138,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 23 ), index = 23  */
    {  6,    0,    0,    0,    144,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 24 ), index = 24  */
    { 25,    0,    0,    2,    150,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 25 ), index = 25  */
    { 13,    0,    0,    1,    156,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 26 ), index = 26  */
    { 27,    0,    0,    2,    162,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 27 ), index = 27  */
    {  7,    0,    0,    0,    168,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 28 ), index = 28  */
    { 29,    0,    0,    2,    174,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 29 ), index = 29  */
    { 15,    0,    0,    1,    180,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 30 ), index = 30  */
    { 31,    0,    0,    2,    186,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 31 ), index = 31  */
    {  8,    0,    0,    0,    192,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 32 ), index = 32  */
    {  8,    0,    0,    0,    192,  CLK_DIV(1, 1, 1, 2) },   /* freq = (6M * 32 ), index = 33  */
    { 17,    0,    0,    1,    204,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 34 ), index = 34  */
    { 17,    0,    0,    1,    204,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 34 ), index = 35  */
    {  9,    0,    0,    0,    216,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 36 ), index = 36  */
    {  9,    0,    0,    0,    216,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 36 ), index = 37  */
    { 19,    0,    0,    1,    228,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 38 ), index = 38  */
    { 19,    0,    0,    1,    228,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 38 ), index = 39  */
    { 10,    0,    0,    0,    240,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 40 ), index = 40  */
    { 10,    0,    0,    0,    240,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 40 ), index = 41  */
    { 21,    0,    0,    1,    252,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 42 ), index = 42  */
    { 21,    0,    0,    1,    252,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 42 ), index = 43  */
    { 11,    0,    0,    0,    264,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 44 ), index = 44  */
    { 11,    0,    0,    0,    264,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 44 ), index = 45  */
    { 23,    0,    0,    1,    276,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 46 ), index = 46  */
    { 23,    0,    0,    1,    276,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 46 ), index = 47  */
    { 12,    0,    0,    0,    288,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 48 ), index = 48  */
    { 12,    0,    0,    0,    288,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 48 ), index = 49  */
    { 25,    0,    0,    1,    300,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 50 ), index = 50  */
    { 25,    0,    0,    1,    300,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 50 ), index = 51  */
    { 13,    0,    0,    0,    312,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 52 ), index = 52  */
    { 13,    0,    0,    0,    312,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 52 ), index = 53  */
    { 27,    0,    0,    1,    324,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 54 ), index = 54  */
    { 27,    0,    0,    1,    324,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 54 ), index = 55  */
    { 14,    0,    0,    0,    336,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 56 ), index = 56  */
    { 14,    0,    0,    0,    336,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 56 ), index = 57  */
    { 29,    0,    0,    1,    348,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 58 ), index = 58  */
    { 29,    0,    0,    1,    348,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 58 ), index = 59  */
    { 15,    0,    0,    0,    360,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 60 ), index = 60  */
    { 15,    0,    0,    0,    360,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 60 ), index = 61  */
    { 31,    0,    0,    1,    372,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 62 ), index = 62  */
    { 31,    0,    0,    1,    372,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 62 ), index = 63  */
    { 16,    0,    0,    0,    384,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 64 ), index = 64  */
    { 16,    0,    0,    0,    384,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 64 ), index = 65  */
    { 16,    0,    0,    0,    384,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 64 ), index = 66  */
    { 16,    0,    0,    0,    384,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 64 ), index = 67  */
    { 17,    0,    0,    0,    408,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 68 ), index = 68  */
    { 17,    0,    0,    0,    408,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 68 ), index = 69  */
    { 17,    0,    0,    0,    408,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 68 ), index = 70  */
    { 17,    0,    0,    0,    408,  CLK_DIV(1, 1, 2, 2) },   /* freq = (6M * 68 ), index = 71  */
    { 18,    0,    0,    0,    432,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 72 ), index = 72  */
    { 18,    0,    0,    0,    432,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 72 ), index = 73  */
    { 18,    0,    0,    0,    432,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 72 ), index = 74  */
    { 18,    0,    0,    0,    432,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 72 ), index = 75  */
    { 19,    0,    0,    0,    456,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 76 ), index = 76  */
    { 19,    0,    0,    0,    456,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 76 ), index = 77  */
    { 19,    0,    0,    0,    456,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 76 ), index = 78  */
    { 19,    0,    0,    0,    456,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 76 ), index = 79  */
    { 20,    0,    0,    0,    480,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 80 ), index = 80  */
    { 20,    0,    0,    0,    480,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 80 ), index = 81  */
    { 20,    0,    0,    0,    480,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 80 ), index = 82  */
    { 20,    0,    0,    0,    480,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 80 ), index = 83  */
    { 21,    0,    0,    0,    504,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 84 ), index = 84  */
    { 21,    0,    0,    0,    504,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 84 ), index = 85  */
    { 21,    0,    0,    0,    504,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 84 ), index = 86  */
    { 21,    0,    0,    0,    504,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 84 ), index = 87  */
    { 22,    0,    0,    0,    528,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 88 ), index = 88  */
    { 22,    0,    0,    0,    528,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 88 ), index = 89  */
    { 22,    0,    0,    0,    528,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 88 ), index = 90  */
    { 22,    0,    0,    0,    528,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 88 ), index = 91  */
    { 23,    0,    0,    0,    552,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 92 ), index = 92  */
    { 23,    0,    0,    0,    552,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 92 ), index = 93  */
    { 23,    0,    0,    0,    552,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 92 ), index = 94  */
    { 23,    0,    0,    0,    552,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 92 ), index = 95  */
    { 24,    0,    0,    0,    576,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 96 ), index = 96  */
    { 24,    0,    0,    0,    576,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 96 ), index = 97  */
    { 24,    0,    0,    0,    576,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 96 ), index = 98  */
    { 24,    0,    0,    0,    576,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 96 ), index = 99  */
    { 25,    0,    0,    0,    600,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 100), index = 100 */
    { 25,    0,    0,    0,    600,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 100), index = 101 */
    { 25,    0,    0,    0,    600,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 100), index = 102 */
    { 25,    0,    0,    0,    600,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 100), index = 103 */
    { 26,    0,    0,    0,    624,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 104), index = 104 */
    { 26,    0,    0,    0,    624,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 104), index = 105 */
    { 26,    0,    0,    0,    624,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 104), index = 106 */
    { 26,    0,    0,    0,    624,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 104), index = 107 */
    { 27,    0,    0,    0,    648,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 108), index = 108 */
    { 27,    0,    0,    0,    648,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 108), index = 109 */
    { 27,    0,    0,    0,    648,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 108), index = 110 */
    { 27,    0,    0,    0,    648,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 108), index = 111 */
    { 14,    1,    0,    0,    672,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 112), index = 112 */
    { 14,    1,    0,    0,    672,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 112), index = 113 */
    { 14,    1,    0,    0,    672,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 112), index = 114 */
    { 14,    1,    0,    0,    672,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 112), index = 115 */
    { 29,    0,    0,    0,    696,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 116), index = 116 */
    { 29,    0,    0,    0,    696,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 116), index = 117 */
    { 29,    0,    0,    0,    696,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 116), index = 118 */
    { 29,    0,    0,    0,    696,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 116), index = 119 */
    { 15,    1,    0,    0,    720,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 120), index = 120 */
    { 15,    1,    0,    0,    720,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 120), index = 121 */
    { 15,    1,    0,    0,    720,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 120), index = 122 */
    { 15,    1,    0,    0,    720,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 120), index = 123 */
    { 31,    0,    0,    0,    744,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 124), index = 124 */
    { 31,    0,    0,    0,    744,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 124), index = 125 */
    { 31,    0,    0,    0,    744,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 124), index = 126 */
    { 31,    0,    0,    0,    744,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 124), index = 127 */
    { 16,    1,    0,    0,    768,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 128), index = 128 */
    { 16,    1,    0,    0,    768,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 128), index = 129 */
    { 16,    1,    0,    0,    768,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 128), index = 130 */
    { 16,    1,    0,    0,    768,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 128), index = 131 */
    { 16,    1,    0,    0,    768,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 128), index = 132 */
    { 16,    1,    0,    0,    768,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 128), index = 133 */
    { 16,    1,    0,    0,    768,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 128), index = 134 */
    { 16,    1,    0,    0,    768,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 128), index = 135 */
    { 17,    1,    0,    0,    816,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 136), index = 136 */
    { 17,    1,    0,    0,    816,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 136), index = 137 */
    { 17,    1,    0,    0,    816,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 136), index = 138 */
    { 17,    1,    0,    0,    816,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 136), index = 139 */
    { 17,    1,    0,    0,    816,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 136), index = 140 */
    { 17,    1,    0,    0,    816,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 136), index = 141 */
    { 17,    1,    0,    0,    816,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 136), index = 142 */
    { 17,    1,    0,    0,    816,  CLK_DIV(1, 2, 2, 2) },   /* freq = (6M * 136), index = 143 */
    { 18,    1,    0,    0,    864,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 144), index = 144 */
    { 18,    1,    0,    0,    864,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 144), index = 145 */
    { 18,    1,    0,    0,    864,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 144), index = 146 */
    { 18,    1,    0,    0,    864,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 144), index = 147 */
    { 18,    1,    0,    0,    864,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 144), index = 148 */
    { 18,    1,    0,    0,    864,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 144), index = 149 */
    { 18,    1,    0,    0,    864,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 144), index = 150 */
    { 18,    1,    0,    0,    864,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 144), index = 151 */
    { 19,    1,    0,    0,    912,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 152), index = 152 */
    { 19,    1,    0,    0,    912,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 152), index = 153 */
    { 19,    1,    0,    0,    912,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 152), index = 154 */
    { 19,    1,    0,    0,    912,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 152), index = 155 */
    { 13,    2,    0,    0,    936,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 152), index = 156 */
    { 13,    2,    0,    0,    936,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 152), index = 157 */
    { 13,    2,    0,    0,    936,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 152), index = 158 */
    { 13,    2,    0,    0,    936,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 152), index = 159 */
    { 20,    1,    0,    0,    960,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 160), index = 160 */
    { 20,    1,    0,    0,    960,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 160), index = 161 */
    { 20,    1,    0,    0,    960,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 160), index = 162 */
    { 20,    1,    0,    0,    960,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 160), index = 163 */
    { 20,    1,    0,    0,    960,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 160), index = 164 */
    { 20,    1,    0,    0,    960,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 160), index = 165 */
    { 20,    1,    0,    0,    960,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 160), index = 166 */
    { 20,    1,    0,    0,    960,  CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 160), index = 167 */
    { 21,    1,    0,    0,    1008, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 168), index = 168 */
    { 21,    1,    0,    0,    1008, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 168), index = 169 */
    { 21,    1,    0,    0,    1008, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 168), index = 170 */
    { 21,    1,    0,    0,    1008, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 168), index = 171 */
    { 21,    1,    0,    0,    1008, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 168), index = 172 */
    { 21,    1,    0,    0,    1008, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 168), index = 173 */
    { 21,    1,    0,    0,    1008, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 168), index = 174 */
    { 21,    1,    0,    0,    1008, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 168), index = 175 */
    { 22,    1,    0,    0,    1056, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 176), index = 176 */
    { 22,    1,    0,    0,    1056, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 176), index = 177 */
    { 22,    1,    0,    0,    1056, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 176), index = 178 */
    { 22,    1,    0,    0,    1056, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 176), index = 179 */
    { 22,    1,    0,    0,    1056, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 176), index = 180 */
    { 22,    1,    0,    0,    1056, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 176), index = 181 */
    { 22,    1,    0,    0,    1056, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 176), index = 182 */
    { 22,    1,    0,    0,    1056, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 176), index = 183 */
    { 23,    1,    0,    0,    1104, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 184), index = 184 */
    { 23,    1,    0,    0,    1104, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 184), index = 185 */
    { 23,    1,    0,    0,    1104, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 184), index = 186 */
    { 23,    1,    0,    0,    1104, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 184), index = 187 */
    { 23,    1,    0,    0,    1104, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 184), index = 188 */
    { 23,    1,    0,    0,    1104, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 184), index = 189 */
    { 23,    1,    0,    0,    1104, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 184), index = 190 */
    { 23,    1,    0,    0,    1104, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 184), index = 191 */
    { 24,    1,    0,    0,    1152, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 192), index = 192 */
    { 24,    1,    0,    0,    1152, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 192), index = 193 */
    { 24,    1,    0,    0,    1152, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 192), index = 194 */
    { 24,    1,    0,    0,    1152, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 192), index = 195 */
    { 24,    1,    0,    0,    1152, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 192), index = 196 */
    { 24,    1,    0,    0,    1152, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 192), index = 197 */
    { 24,    1,    0,    0,    1152, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 192), index = 198 */
    { 24,    1,    0,    0,    1152, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 192), index = 199 */
    { 25,    1,    0,    0,    1200, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 200), index = 200 */
    { 25,    1,    0,    0,    1200, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 200), index = 201 */
    { 25,    1,    0,    0,    1200, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 200), index = 202 */
    { 25,    1,    0,    0,    1200, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 200), index = 203 */
    { 25,    1,    0,    0,    1200, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 200), index = 204 */
    { 25,    1,    0,    0,    1200, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 200), index = 205 */
    { 25,    1,    0,    0,    1200, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 200), index = 206 */
    { 25,    1,    0,    0,    1200, CLK_DIV(1, 3, 2, 2) },   /* freq = (6M * 200), index = 207 */
    { 26,    1,    0,    0,    1248, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 208), index = 208 */
    { 26,    1,    0,    0,    1248, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 208), index = 209 */
    { 26,    1,    0,    0,    1248, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 208), index = 210 */
    { 26,    1,    0,    0,    1248, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 208), index = 211 */
    { 26,    1,    0,    0,    1248, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 208), index = 212 */
    { 26,    1,    0,    0,    1248, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 208), index = 213 */
    { 26,    1,    0,    0,    1248, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 208), index = 214 */
    { 26,    1,    0,    0,    1248, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 208), index = 215 */
    { 27,    1,    0,    0,    1296, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 216), index = 216 */
    { 27,    1,    0,    0,    1296, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 216), index = 217 */
    { 27,    1,    0,    0,    1296, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 216), index = 218 */
    { 27,    1,    0,    0,    1296, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 216), index = 219 */
    { 27,    1,    0,    0,    1296, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 216), index = 220 */
    { 27,    1,    0,    0,    1296, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 216), index = 221 */
    { 27,    1,    0,    0,    1296, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 216), index = 222 */
    { 27,    1,    0,    0,    1296, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 216), index = 223 */
    { 28,    1,    0,    0,    1344, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 224), index = 224 */
    { 28,    1,    0,    0,    1344, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 224), index = 225 */
    { 28,    1,    0,    0,    1344, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 224), index = 226 */
    { 28,    1,    0,    0,    1344, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 224), index = 227 */
    { 28,    1,    0,    0,    1344, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 224), index = 228 */
    { 28,    1,    0,    0,    1344, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 224), index = 229 */
    { 28,    1,    0,    0,    1344, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 224), index = 230 */
    { 28,    1,    0,    0,    1344, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 224), index = 231 */
    { 29,    1,    0,    0,    1392, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 232), index = 232 */
    { 29,    1,    0,    0,    1392, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 232), index = 233 */
    { 29,    1,    0,    0,    1392, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 232), index = 234 */
    { 29,    1,    0,    0,    1392, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 232), index = 235 */
    { 29,    1,    0,    0,    1392, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 232), index = 236 */
    { 29,    1,    0,    0,    1392, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 232), index = 237 */
    { 29,    1,    0,    0,    1392, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 232), index = 238 */
    { 29,    1,    0,    0,    1392, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 232), index = 239 */
    { 30,    1,    0,    0,    1440, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 240), index = 240 */
    { 30,    1,    0,    0,    1440, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 240), index = 241 */
    { 30,    1,    0,    0,    1440, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 240), index = 242 */
    { 30,    1,    0,    0,    1440, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 240), index = 243 */
    { 30,    1,    0,    0,    1440, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 240), index = 244 */
    { 30,    1,    0,    0,    1440, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 240), index = 245 */
    { 30,    1,    0,    0,    1440, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 240), index = 246 */
    { 30,    1,    0,    0,    1440, CLK_DIV(1, 4, 2, 2) },   /* freq = (6M * 240), index = 247 */
    { 31,    1,    0,    0,    1488, CLK_DIV(1, 4, 2, 2) }    /* freq = (6M * 248), index = 248 */
};
static int clk_get_pll_para(struct core_pll_freq_tbl *factor, int rate);
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int sunxi_clock_get_corepll(void)
{
	
	__u32 reg_val;
	__u32 div_p, factor_n;
	__u32 factor_k, factor_m;
	__u32 clock;

	reg_val  = readl(CCM_PLL1_CPUX_CTRL);
	div_p    = 1 << ((reg_val >> 16) & 0x3);
	factor_n = (reg_val >> 8) & 0x1f;
	if(factor_n == 0)
        factor_n = 1;
	factor_k = ((reg_val >> 4) & 0x3) + 1;
	factor_m = ((reg_val >> 0) & 0x3) + 1;

	clock = 24 * factor_n * factor_k/div_p/factor_m;

	return clock;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int sunxi_clock_get_axi(void)
{
	
	__u32 clock;
	__u32 reg_val;
	__u32 clock_src, factor;

	reg_val   = readl(CCM_AHB1_APB1_CTRL);
	clock_src = (reg_val >> 16) & 0x03;
	factor    = ((reg_val >> 0) & 0x03) + 1;

	switch(clock_src)
	{
		case 0:
			clock = 32000;
			break;
		case 1:
			clock = 24;
			break;
		case 2:
			clock =  sunxi_clock_get_corepll();
			break;
		default:
			return 0;
	}

	return clock/factor;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int sunxi_clock_get_ahb(void)
{


	unsigned int reg_val;
	int factor;
	int clock;

	reg_val = readl(CCM_AHB1_APB1_CTRL);
	factor  = (reg_val >> 4) & 0x03;
	clock   = sunxi_clock_get_axi()>>factor;

	return clock;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int sunxi_clock_get_apb1(void)
{
	unsigned int reg_val;
	int          clock, factor;

	reg_val = readl(CCM_AHB1_APB1_CTRL);
	factor  = (reg_val >> 8) & 0x03;
	clock   = sunxi_clock_get_ahb();

	if(factor)
	{
		clock >>= factor;
	}
	else
	{
		clock >>= 1;
	}

	return clock;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int sunxi_clock_get_apb2(void)
{
	__u32 clock = 0;
	__u32 factor;

	factor = (readl(CCM_AHB1_APB1_CTRL) >> 24) & 0x03;
	if(factor == 0)
	{
		clock = 24000;
	}

	return clock;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*
*
*    ����ֵ  ��
*
*    ˵��    ��ֻ���ڵ���COREPLL���̶���Ƶ�ȣ�4:2:1
*
*
************************************************************************************************************
*/
static int clk_get_pll_para(struct core_pll_freq_tbl *factor, int pll_clk)
{
	
	int     index;

    if(pll_clk > 1488)
    {
        pll_clk = 1488;
    }
    index = pll_clk/6;

    factor->FactorN = CorePllTbl[index].FactorN;
    factor->FactorK = CorePllTbl[index].FactorK;
    factor->FactorM = CorePllTbl[index].FactorM;
    factor->FactorP = CorePllTbl[index].FactorP;
    factor->clk_div = CorePllTbl[index].clk_div;

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
static int clk_set_divd(int pll)
{
	unsigned int reg_val;

	//config axi
	/*
	reg_val = readl(CCM_CPU_L2_AXI_CTRL);
	reg_val &= ~(0x03 << 0);
	reg_val |=  (0x02 << 0);
	writel(reg_val, CCM_CPU_L2_AXI_CTRL);
	*/
	//config ahb
	reg_val = readl(CCM_AHB1_APB1_CTRL);;
	reg_val &= ~((0x03 << 9)|(0x03 << 4)|(0x03 << 0));
	reg_val |= ((0x01 << 9)|(0x01 << 4)|(0x01 << 0));
	writel(reg_val, CCM_AHB1_APB1_CTRL);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*
*
*    ����ֵ  ��
*
*    ˵��    ��ֻ���ڵ���COREPLL���̶���Ƶ�ȣ�4:2:1
*
*
************************************************************************************************************
*/

__u32 sunxi_clock_set_corepll(__u32 clock_frequency, __u32 core_vol)
{
    __u32 reg_val;
    __u32 i;
    struct core_pll_freq_tbl  pll_factor;
    //���ʱ���Ƿ�Ϸ�
    if((!clock_frequency) || (clock_frequency > 2 * 1024 * 1024))
    {
        //Ĭ��Ƶ��
        clock_frequency = 384;
    }
    else if(clock_frequency < 24)
    {
        //����ʱ�Ӳ���24M����������Ĭ��Ƶ��
        clock_frequency = 24;
        //�л���24M
        reg_val = readl(CCM_AHB1_APB1_CTRL);
        reg_val &= ~(0x03 << 16);
        reg_val |=  (0x01 << 16);
        writel(CCM_AHB1_APB1_CTRL,reg_val);

        return 24;
    }
    else
    {
		for(i=0;;i++)
		{
			if(core_vol >= volt_freq_table[i][1])
			{
				if(clock_frequency >= volt_freq_table[i][0])
				{
					clock_frequency = volt_freq_table[i][0];
				}
				break;
			}
			if(!volt_freq_table[i][0])
			{
				clock_frequency = 384;
				break;
			}
		}
    }
    
    reg_val = readl(CCM_AHB1_APB1_CTRL);
    reg_val &= ~(0x03 << 16);
    reg_val |=  (0x01 << 16);
    writel(CCM_AHB1_APB1_CTRL,reg_val);
    //��ʱ���ȴ�ʱ���ȶ�
    for(i=0; i<0x4000; i++);
    //����ʱ��Ƶ��
	clk_get_pll_para(&pll_factor, clock_frequency);
	
    reg_val = readl(CCM_PLL1_CPUX_CTRL);
    reg_val &= ~((0x1f << 8) | (0x03 << 4) | (0x03 << 16) | (0x03 << 0));
	reg_val |=  (pll_factor.FactorN<<8) | (pll_factor.FactorK<<4) | (pll_factor.FactorP << 16) | (pll_factor.FactorM << 0);
    writel(reg_val, CCM_PLL1_CPUX_CTRL);
    //�޸�AXI,AHB,APB��Ƶ
    reg_val = readl(CCM_AHB1_APB1_CTRL);
    reg_val &= ~(0x3ff << 0);
    reg_val |=  (pll_factor.clk_div << 0);
    //�޸�ATB
    reg_val &= ~(0x03<<2);
    reg_val |=  (0x02<<2);
    writel(reg_val, CCM_AHB1_APB1_CTRL);
    //��ʱ���ȴ�ʱ���ȶ�
    for(i=0; i<0x4000; i++);
    //�л�ʱ�ӵ�COREPLL��
    reg_val = readl(CCM_AHB1_APB1_CTRL);
    reg_val &= ~(0x03 << 16);
    reg_val |=  (0x02 << 16);
    writel(reg_val, CCM_AHB1_APB1_CTRL);

    return  sunxi_clock_get_corepll();
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int sunxi_clock_set_pll6(void)
{
    int i;
	unsigned int reg_val;

    /* set voltage and ldo for pll */

    reg_val = readl((SUNXI_R_PRCM_REGS_BASE+0x44));
    reg_val &= ~(0xff << 24);
    reg_val |= 0xa7 << 24;
    writel(reg_val, (SUNXI_R_PRCM_REGS_BASE+0x44));
    reg_val = readl((SUNXI_R_PRCM_REGS_BASE+0x44));
    reg_val &= ~(0x1 << 15);
    reg_val &= ~(0x7 << 16);
    reg_val |= 0x7 << 16;
    writel(reg_val, (SUNXI_R_PRCM_REGS_BASE+0x44));
    /* delaly some time*/
    for(i=0; i<100000; i++);
    
    /* set pll6 frequency to 600Mhz, and enable it */
	writel(0x80041811, CCM_PLL6_MOD_CTRL);
	do
	{
		reg_val = readl(CCM_PLL6_MOD_CTRL);
	}
	while(!(reg_val & (0x1 << 28)));

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int sunxi_clock_set_mbus(void)
{
	int factor_n, factor_k, pll6;
	unsigned int reg_val;

    /* set voltage and ldo for pll */
	reg_val = readl(CCM_PLL6_MOD_CTRL);
	factor_n = ((reg_val >> 8) & 0x1f) + 1;
	factor_k = ((reg_val >> 4) & 0x03) + 1;
	pll6 = 24 * factor_n * factor_k/2;

    if(pll6 > 300 * 4) {
        factor_n = 5;
    } else if(pll6 > 300*3){
        factor_n = 4;
    } else if(pll6 > 300*2){
        factor_n = 3;
    } else if(pll6 > 300*1){
        factor_n = 2;
    } else {
        factor_n = 1;
    }

    /* config mbus0 */
    writel((0x81000000|(factor_n-1)), CCM_MBUS_SCLK_CTRL0);
    writel((0x81000000|(factor_n-1)), CCM_MBUS_SCLK_CTRL1);

    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
int sunxi_clock_get_pll6(void)
{
	unsigned int reg_val;
	int factor_n, factor_k, pll6;

	reg_val = readl(CCM_PLL6_MOD_CTRL);
	factor_n = ((reg_val >> 8) & 0x1f) + 1;
	factor_k = ((reg_val >> 4) & 0x03) + 1;
	pll6 = 24 * factor_n * factor_k/2;

	return pll6;
}
