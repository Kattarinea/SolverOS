// Эта инструкция обязательно должна быть первой, т.к. этот код компилируется в бинарный,
// и загрузчик передает управление по адресу первой инструкции бинарного образа ядра ОС.
__asm("jmp kmain");
#define VIDEO_BUF_PTR (0xb8000)

void keyb_handler();
void keyb_process_keys();

#define INT_MAX = 2147483647

int color_T;
const char* str_color;
char str_cat[36] = { 0 };
int cursor_position_x = 2;
int cursor_position_y;
const char* info = "info";
const char* gcd = "gcd";
const char* uravn = "solve equation";
const char* shutdown = "shutdown";
int length_str_ur = 13;
int length_str_info = 4;
int length_str_gcd = 3;
int length_str_shutdown = 8;
int end_of_command = 0;

char vvod[40] = { 0 };
int index_of_number = 0;
int flag_number = 0;
int index_vvod = 0;
int first_for_NOD = 0;
int seconf_for_NOD = 0;
int plus_pos = 0;

void cursor_moveto(unsigned int strnum, unsigned int pos);
char table_for_letter[] = { 0,0,'1','2','3','4','5','6','7','8','9','0', '-', '=',' ', 0,'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0, 0,0,0,
	   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0,0,0,0, 0, 'z', 'x', 'c', 'v', 'b', 'n', 'm',0, 0, '/', 0, '*', 0, ' ',0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0,  0, '-', 0, 0, 0,'+'
};

int last_cursor_y_pos = 0;
int last_cursor_x_pos = 0;

void out_str(int color, const char* ptr, unsigned int strnum)
{

	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	video_buf += 80 * 2 * strnum;
	while (*ptr)
	{
		video_buf[0] = (unsigned char)* ptr; // Символ (код)

		video_buf[1] = color; // Цвет символа и фона
		video_buf += 2;
		ptr++;
	}
}

int out_my_letter(int color, int str, unsigned int coloms)
{
	int metka = 0;
	if (str == 28) { cursor_moveto(cursor_position_y, 0); cursor_position_x = 0; return 0; }
	else   //enter
		if (str == 14) { str = (int)table_for_letter[14]; metka = 1; cursor_moveto(cursor_position_y, coloms); }   //bs
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	video_buf += 80 * 2 * cursor_position_y + 2 * coloms;

	video_buf[0] = (unsigned char)str; // Символ (код)

	video_buf[1] = color; // Цвет символа и фона
	if (metka != 1)
		cursor_moveto(cursor_position_y, coloms);


}

#define IDT_TYPE_INTR (0x0E)
#define IDT_TYPE_TRAP (0x0F)
// Селектор секции кода, установленный загрузчиком ОС
#define GDT_CS (0x8)
// Структура описывает данные об обработчике прерывания
struct idt_entry
{
	unsigned short base_lo; // Младшие биты адреса обработчика
	unsigned short segm_sel; // Селектор сегмента кода
	unsigned char always0; // Этот байт всегда 0
	unsigned char flags; // Флаги тип. Флаги: P, DPL, Типы - это константы - IDT_TYPE...
	unsigned short base_hi; // Старшие биты адреса обработчика
} __attribute__((packed)); // Выравнивание запрещено
// Структура, адрес которой передается как аргумент команды lidt
struct idt_ptr
{
	unsigned short limit;
	unsigned int base;
} __attribute__((packed)); // Выравнивание запрещено
struct idt_entry g_idt[256]; // Реальная таблица IDT
struct idt_ptr g_idtp; // Описатель таблицы для команды lidt
// Пустой обработчик прерываний. Другие обработчики могут быть реализованы по этому шаблону
void default_intr_handler()
{
	asm("pusha");
	// ... (реализация обработки)
	asm("popa; leave; iret");
}
typedef void (*intr_handler)();
void intr_reg_handler(int num, unsigned short segm_sel, unsigned short
	flags, intr_handler hndlr)
{
	unsigned int hndlr_addr = (unsigned int)hndlr;
	g_idt[num].base_lo = (unsigned short)(hndlr_addr & 0xFFFF);
	g_idt[num].segm_sel = segm_sel;
	g_idt[num].always0 = 0;
	g_idt[num].flags = flags;
	g_idt[num].base_hi = (unsigned short)(hndlr_addr >> 16);
}
// Функция инициализации системы прерываний: заполнение массива с адресамиобработчиков
void intr_init()
{
	int i;
	int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
	for (i = 0; i < idt_count; i++)
		intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR,
			default_intr_handler); // segm_sel=0x8, P=1, DPL=0, Type=Intr
}
void intr_start()
{
	int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
	g_idtp.base = (unsigned int)(&g_idt[0]);
	g_idtp.limit = (sizeof(struct idt_entry) * idt_count) - 1;
	asm("lidt %0" : : "m" (g_idtp));
}
void intr_enable()
{
	asm("sti");
}
void intr_disable()
{
	asm("cli");
}

static inline unsigned char inb(unsigned short port) // Чтение из порта
{
	unsigned char data;
	asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
	return data;
}
static inline void outb(unsigned short port, unsigned char data) // Запись
{
	asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));
}
static inline void outw(unsigned short port, unsigned int data)
{
	/* See [IA32-v2b] "OUT". */
	asm volatile ("outw %w0, %w1" : : "a" (data), "Nd" (port));
}


#define PIC1_PORT (0x20)
void keyb_init()
{
	// Регистрация обработчика прерывания
	intr_reg_handler(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyb_handler);
	// segm_sel=0x8, P=1, DPL=0, Type=Intr
	// Разрешение только прерываний клавиатуры от контроллера 8259
	outb(PIC1_PORT + 1, 0xFF ^ 0x02); // 0xFF - все прерывания, 0x02 - бит IRQ1 (клавиатура).
	// Разрешены будут только прерывания, чьи биты установлены в 0
}

void keyb_handler()
{
	asm("pusha");
	// Обработка поступивших данных
	keyb_process_keys();
	// Отправка контроллеру 8259 нотификации о том, что прерывание обработано
	outb(PIC1_PORT, 0x20);
	asm("popa; leave; iret");
}

void info_function()
{
	const char* str = "Solver OS: v.01. Developer: Shapochnik Ekaterina, 4831001/90001, SpbPU, 2021";
	out_str(color_T, str, cursor_position_y);
	cursor_position_y++;
	str = "Compilers: bootloader: yasm, kernel: gcc";
	out_str(color_T, str, cursor_position_y);
	cursor_position_y++;
	str = "Bootloader parameters: ";
	int i = 0;
	for (; i < 22; i++)
		str_cat[i] = str[i];

	str_cat[i] = ' '; i++;
	int in = 0;
	for (; str_color[in] != 0; i++, in++)
		str_cat[i] = str_color[in];
	in = 0; str = " color.";
	for (i; in < 6; i++, in++)
		str_cat[i] = *(str + in);
	str = str_cat;

	out_str(color_T, str, cursor_position_y);
	cursor_position_y++;

}
int gcd_function(int a, int b)
{
	int c;
	while (b) {
		c = a % b;
		a = b;
		b = c;
	}
	if (a < 0)a = a * (-1);
	return a;
}
char res[40] = { 0 };
int count_function(int result, int i)
{
	int dop = 0; dop = result;
	int count = 0;
	while (dop > 0)
	{
		count++;
		dop = dop / 10;

	}
	count--;
	while (count >= 0)
	{
		int divv = 1;
		for (int n = 0; n < count; n++)
			divv = divv * 10;

		if (count == 0)
		{
			res[i] = result + 48;
			i++;
		}
		else
		{

			res[i] = (result / divv) + 48;
			result = result % divv;
			i++;
		}
		count--;
	}
return i;
}
void error()
{
	out_str(color_T, "Error:  command incorrect", cursor_position_y);
	cursor_position_y++;
	cursor_position_x = 2;

	out_str(color_T, "#", cursor_position_y);
	cursor_moveto(cursor_position_y, cursor_position_x);
	for (int i = 0; i < 40; i++)vvod[i] = 0;

}
void error_ov()
{
	out_str(color_T, "Error:  Integer overflow", cursor_position_y);
	cursor_position_y++;
	cursor_position_x = 2;

	out_str(color_T, "#", cursor_position_y);
	cursor_moveto(cursor_position_y, cursor_position_x);
	for (int i = 0; i < 40; i++)vvod[i] = 0;

}
int err=0;
void solve_equation_function(int a, int b, int c)
{
	int mines_res = 0; int mines_res2 = 0;
	int result = 0;

	if(c>0 && b<0 && (c-b)<0 || b>0 && c<0 && (c-b)>0){error_ov();err++;}
	else
	{

	result = c - b;
	if (result < 0 && a>0) { mines_res = 1; result = result * (-1); }
	else
		if (result > 0 && a < 0) { mines_res = 1; a = a * (-1); }
		else
			if (result < 0 && a < 0) { result = result * (-1); a = a * (-1);  }
	

	const char* str = "Result: x=";

	int i = 0;
	for (; i < 11; i++)
		res[i] = str[i];

	int NOD = gcd_function(result, a);
	
	if (NOD == a) {
		result = result / a; 
		int dop = 0; dop = result;
		if (mines_res == 1) { res[i] = '-'; i++; }
		if (result < 10)
		{
			res[i] = result + 48;
			i++;
		}
		else
		{	i=count_function(result, i);}
	}
	else
		if (NOD == result)
		{
			if (mines_res == 1) { res[i] = '-'; i++; }
			res[i] = '1'; i++; res[i] = '/'; i++;
			result = a / result;
			if (result < 10)
			{
				res[i] = result + 48;
				i++;
			}
			else
			{
				i=count_function(result, i);


			}
		}
		else
		{
			int first = result / NOD;
			int second = a / NOD;
			if (mines_res == 1) { res[i] = '-'; i++; }
			if (first < 10)
			{
				res[i] = first + 48;
				i++;
			}
			else
			{
				i=count_function(first, i);


			}
			res[i] = '/';
			i++;
			if (second < 10)
			{
				res[i] = second + 48;
				i++;
			}
			else
			{
				i=count_function(second, i);



			}


		}
	for (int j = 0; j < i; j++)
		out_my_letter(color_T, res[j], j);
	//out_str(color_T, str, cursor_position_y);
	}

}

int cmp_str(char* vvod, const char* data, int length)
{
	if (length == length_str_ur && length != (end_of_command + 1))
	{
		const char* a = "solve";
		for (int i = 0; i < 5; i++)
			if (vvod[i] != a[i])return 0;
		return 1;
	}
	else
	{
		for (int i = 0; i < length; i++)
			if (vvod[i] != data[i])return 0;

		if (length != (end_of_command + 1)) return 0;

		return 1;
	}
}

int check_args(int index, int str_command)
{
	//out_str(color_T, "                                                              ", 20);
	int a_count=0;int b_count=0;
	if ((str_command == 1 || str_command == 4) && index == 0)//shutdown and info
		return 1;
	if (str_command == 2)   //gcd
	{
		if (index_of_number == 0)return 0;
		for (int i = index; i < index_vvod; i++)
		{
			if (vvod[i] == '-' || vvod[i] == '=' || ((vvod[i] < 49 || vvod[i]>57)) && vvod[i] != ' ') { return 0; }
		}
		int a = 0; int b = 0; int space = 0;
		for (int i = index; i < index_vvod; i++)
		{
			if (vvod[i] != ' ')
			{
				if (space == 0)
				{
					a = (vvod[i] - 48) + a * 10;a_count++;

				}
				else
					b = (vvod[i] - 48) + b * 10;b_count++;
			}
			else
			{
				space++;
			}
		}

		if (a <= 0 || b <= 0 ||a_count>10 || b_count>10) { error_ov(); return 2; }
		else return 1;

	}
	if (str_command == 3)//solve
	{int new_ind=0;
        if(index_of_number<13)
        {new_ind =5;if (vvod[5]!=' ')return 0;}
        else {new_ind=14;if (vvod[14]!=' ')return 0;}
		int count_x = 0; int count_plus = 0; int count_ravno = 0; int no_such_symbol = 0; int r_pos = 0; int pl_pos = 0;int count_twice=0;int flag=0;int count_space=0;int num=0;int min=0;int pos_x=0;int min_pos=0;
		for (int i = new_ind; i < index_vvod; i++)
		{  if(vvod[i]==' ' && flag!=0)count_space=i;
		if(vvod[i]=='-'){if(r_pos!=0)min++;}
             if((vvod[i]=='+'|| vvod[i]=='='||vvod[i]=='-')){flag=i;if(vvod[i]=='-' && r_pos==0 )count_twice++;else if(min>1 || vvod[i]=='=' || vvod[i]=='+')count_twice++;}
			 
             if((vvod[i] > 47 && vvod[i]<58 || vvod[i]=='x') && count_twice<2){count_space=0;flag=0;count_twice=0;if(r_pos==0)num=i;}
			if (vvod[i] == '+') { count_plus++; pl_pos = i; }
			else
				if (vvod[i] == '=') { count_ravno++; r_pos = i; }
				else
					if (vvod[i] == 'x'){count_x++;pos_x=i;}
					else
						if ((vvod[i] < 48 || vvod[i]>57)&& vvod[i]!='-' && vvod[i]!=' ') {no_such_symbol++;}
		}
        
		if (count_x > 1 || count_plus > 1 || count_ravno > 1 || no_such_symbol != 0 || r_pos < pl_pos || count_x == 0 || count_ravno == 0||  count_twice>1 ){return 0;}
		else return 1;
	}

}

int flag_space = 0;
int kol = 0; int behind_space = 0;

void shutdown_function(void)
{

	outw(0xB004, 0x2000); // qemu < 1.7, ex. 1.6.2
	outw(0x604, 0x2000);
}



void on_key(unsigned char scan)
{
	int prohod = 0;



	if (cursor_position_x == 40) { prohod = 2; }
	else
		if (cursor_position_y >= 24 && (scan == 28 || cursor_position_x == 81)) //clean
		{
			const char* b = "                                                                                ";
			for (int i = 1; i < 25; i++)
				out_str(0x07, b, i);

			cursor_position_y = 0;
			cursor_position_x = 0;
		}


	if ((scan < 14 && scan>1 || scan > 15 && scan < 26 || scan>28 && scan < 39 || scan>43 && scan < 51) && prohod != 2)
	{
		flag_space = 1;


		if (flag_number == 0 && scan < 14 && scan>1) { index_of_number = index_vvod; flag_number++; }
		if (flag_number == 0)end_of_command = index_vvod - kol;
		vvod[index_vvod] = table_for_letter[scan];
		index_vvod++;

		out_my_letter(color_T, (int)table_for_letter[scan], cursor_position_x);


		cursor_position_x++;
	}
	else
		if (scan == 28)//enter
		{
			//out_str(color_T, "                                                              ", 17);
		//	out_str(color_T, "                                                              ", 16);
			//out_str(color_T, vvod, 22);
			cursor_position_y++; last_cursor_x_pos = cursor_position_x; prohod = 0;
			out_my_letter(color_T, 28, cursor_position_x);
			if (cmp_str(vvod, shutdown, length_str_shutdown) == 1)
			{
				int check_command = check_args(index_of_number, 1);
				if (check_command == 1)
				{
					shutdown_function();
					cursor_position_y++;
					cursor_position_x = 2;
					for (int i = 0; i < 40; i++)vvod[i] = 0;
					out_str(color_T, "#", cursor_position_y);
					cursor_moveto(cursor_position_y, cursor_position_x);
				}
				else error();

			}
			else
				if (cmp_str(vvod, info, length_str_info) == 1)
				{

					int check_command = check_args(index_of_number, 1);
					if (check_command == 1)
					{
						info_function();
						cursor_position_y++;
						cursor_position_x = 2;
						for (int i = 0; i < 40; i++)vvod[i] = 0;
						out_str(color_T, "#", cursor_position_y);
						cursor_moveto(cursor_position_y, cursor_position_x);
					}
					else error();


				}
				else
				{

					if (cmp_str(vvod, gcd, length_str_gcd) == 1)
					{

						int check_command = check_args(index_of_number, 2);

						if (check_command != 1 )
						{if(check_command!=2)
							error();
						}
						else
						{
							int space = 0;
							for (int i = index_of_number; i < index_vvod; i++)
							{
								if (vvod[i] != ' ')
								{
									if (space == 0)
									{
										first_for_NOD = (vvod[i] - 48) + first_for_NOD * 10;

									}
									else
										seconf_for_NOD = (vvod[i] - 48) + seconf_for_NOD * 10;
								}
								else
								{
									space++;
								}
							}
							for (int i = 0; i < 40; i++)vvod[i] = 0;

							int result = 0;
							result = gcd_function(first_for_NOD, seconf_for_NOD);
							char res[40] = { 0 };
							const char* str = "Result: ";

							int i = 0;
							for (; i < 8; i++)
								res[i] = str[i];

							if (result < 10)
							{
								res[i] = result + 48;
								i++;
							}
							else
							{
								int dop = 0; dop = result;
								int count = 0;
								while (dop > 0)
								{
									count++;
									dop = dop / 10;

								}
								count--;
								while (count >= 0)
								{
									int divv = 1;
									for (int n = 0; n < count; n++)
										divv = divv * 10;

									if (count == 0)
									{
										res[i] = result + 48;
										i++;
									}
									else
									{

										res[i] = (result / divv) + 48;
										result = result % divv;
										i++;
									}
									count--;
								}

							}
							str = res;
							out_str(color_T, str, cursor_position_y);

							int in = 0;
							cursor_position_y++;
							cursor_position_x = 2;

							out_str(color_T, "#", cursor_position_y);
							cursor_moveto(cursor_position_y, cursor_position_x);
						}
					}
					else
					{
						int mines1_pos = 0;
						int mines2_pos = 0;
						int mines3_pos = 0; int mines = 0;
						int otr_a=0;int otr_b=0;int otr_c=0;
						if (cmp_str(vvod, uravn, length_str_ur) == 1)
						{

							int check_command = check_args(index_of_number, 3);
							if (check_command != 1) error();
							else
							{
								int j = 0; int k = 0; int x_pos = 0; int e = 0;
								for (int i = 5; i < index_vvod; i++)
								{
									if (vvod[i] == '-') {
										mines++;
										if (mines1_pos == 0)
											mines1_pos = i;
										else
											if (mines2_pos == 0)
												mines2_pos = i;
											else
												if (mines3_pos == 0)
													mines3_pos = i;
									}
									if (vvod[i] == 'x')
									{
										x_pos = i;
										for (j = i - 1; j > 4; j--)
											if (vvod[j] == ' ' || vvod[j] == '+' || vvod[j] == '-') {
												break;
											}
									}
									if (vvod[i] == '=')
									{
										k = i;
									}

								}

								int r = j+1;
								int ravno_pos = k;int a_count=0;int b_count=0;int c_count=0;
								int arg_a = 0; int arg_b = 0; int arg_c = 0; j++;
								if (vvod[x_pos - 1] == ' ' || vvod[x_pos - 1] == '+' || vvod[x_pos - 1] == '-') { if (mines1_pos != 0 && mines1_pos < plus_pos && mines1_pos < x_pos && mines == 1 || (mines1_pos != 0 && mines1_pos < x_pos && mines == 1 && mines1_pos != index_of_number || mines2_pos != 0 && mines2_pos < x_pos) && plus_pos == 0){arg_a = -1;otr_a++;} else arg_a = 1; a_count=1; }
								else
								{
                                   
									while (vvod[j] != 'x')
									{
										arg_a = vvod[j] - 48 + arg_a * 10;a_count++;
										j++;
									}//if(vvod[r]=='-' || vvod[r-1]=='-') {}
									if (mines1_pos != 0 && mines1_pos < plus_pos && mines1_pos < x_pos || (mines1_pos != 0 && mines1_pos < x_pos && (mines1_pos != index_of_number || mines2_pos != 0 && mines2_pos < ravno_pos)) && plus_pos == 0) { arg_a = arg_a * (-1);otr_a++; }
								}int f = 0;
								for (int i = k + 1; i < index_vvod; i++)
								{
									if (vvod[i] == '-')f++;
									else
										if (vvod[i] != ' ')
											{arg_c = vvod[i] - 48 + arg_c * 10;c_count++;}
								}
								if (f != 0) {
									arg_c = arg_c * (-1); otr_c++;
								}
								if (plus_pos != 0)
								{
									if (x_pos > plus_pos)
									{
										for (int i = index_of_number; i < plus_pos; i++)
											if (vvod[i] != ' ' && vvod[i] != '-' && vvod[i] != '+' && vvod[i] != '=')
												{arg_b = vvod[i] - 48 + arg_b * 10;b_count++;}
									}
									else
									{
										for (int i = plus_pos + 1; i < k; i++)
											if (vvod[i] != ' ')
											{	arg_b = vvod[i] - 48 + arg_b * 10;b_count++;}



										if (mines1_pos != 0 && mines1_pos == index_of_number && (x_pos > plus_pos || plus_pos == 0 && (mines2_pos != 0 && mines2_pos < ravno_pos)) || (mines1_pos != 0 && mines1_pos > x_pos && mines1_pos < ravno_pos) || mines1_pos != 0 && mines2_pos > x_pos && mines2_pos < ravno_pos) { arg_b = arg_b * (-1); otr_b++; }
									}
								}
								else
								{
									int h = index_of_number;//out_str(color_T,"++++",11);
									if (arg_a < 0 && mines1_pos != index_of_number)
									{
										while (vvod[h] != ' ' && vvod[h] != '-' && vvod[h] != '+' && vvod[h] != '=')
										{
											arg_b = vvod[h] - 48 + arg_b * 10;b_count++;
											h++;
										}
									}
									else
										if (arg_a < 0 && mines2_pos != 0 && mines2_pos<ravno_pos && mines2_pos>x_pos || arg_a>0 && mines1_pos != 0 && mines1_pos<ravno_pos && mines1_pos>x_pos)
										{
											if (arg_a < 0)
												h = mines2_pos + 1;
											else h = mines1_pos + 1;

											while (vvod[h] != ' ' && vvod[h] != '-' && vvod[h] != '+' && vvod[h] != '=')
											{
												arg_b = vvod[h] - 48 + arg_b * 10;
												h++;
											}
											arg_b = arg_b * (-1);otr_b++;

										}

								}
                                 if(vvod[r]=='0')error();
								 else
								 if( otr_a==0 && arg_a<0 || a_count>10 || b_count>10 || c_count>10  || otr_b==0 && arg_b<0 || otr_c==0 && arg_c<0)error_ov();
                                 else
								{solve_equation_function(arg_a, arg_b, arg_c);
								if(err==0)
								{
								cursor_position_y++;
								cursor_position_x = 2;

								out_str(color_T, "#", cursor_position_y);
								cursor_moveto(cursor_position_y, cursor_position_x);
								}
								for (int i = 0; i < 40; i++)vvod[i] = 0;
                                }
							}
						}

						else error();
					}
				}
			index_vvod = 0;

			first_for_NOD = 0; seconf_for_NOD = 0;  flag_number = 0; end_of_command = 0;
			index_of_number = 0; kol = 0; flag_space = 0; behind_space = 0; plus_pos = 0;err=0;

		}
		else
			if (scan == 0x39)//space
			{


				if (flag_space != 0)
				{
					vvod[index_vvod] = ' ';
					index_vvod++; kol++;
				}
				else behind_space++;
				cursor_position_x++; cursor_moveto(cursor_position_y, cursor_position_x);

			}
			else
				if (scan == 14)//BackSpace
				{
					index_vvod--; vvod[index_vvod] = 0;
					if (cursor_position_x == 2)
					{
						index_vvod = 0;
						for (int i = 0; i < index_vvod; i++)vvod[i] = 0;

					}
					else
					{
						if (cursor_position_x == 2 && prohod != 1 && prohod != 2) { cursor_position_x = last_cursor_x_pos; last_cursor_x_pos = 2; cursor_position_y--; prohod = 1; }
						else
							cursor_position_x--;
						out_my_letter(color_T, 14, cursor_position_x);



					}

				}
				else
					if (scan == 0x4e)//+
					{
						plus_pos = cursor_position_x - 2 - behind_space;
						vvod[index_vvod] = '+'; index_vvod++;
						out_my_letter(color_T, (int)table_for_letter[78], cursor_position_x);
						cursor_position_x++; cursor_moveto(cursor_position_y, cursor_position_x);
					}
}
void keyb_process_keys()
{
	// Проверка что буфер PS/2 клавиатуры не пуст (младший бит присутствует)
	if (inb(0x64) & 0x01)
	{
		unsigned char scan_code;
		unsigned char state;
		scan_code = inb(0x60); // Считывание символа с PS/2 клавиатуры
		if (scan_code < 128) // Скан-коды выше 128 - это отпускание клавиши
			on_key(scan_code);
	}
}


// Базовый порт управления курсором текстового экрана. Подходит для большинства, но может отличаться в других BIOS и в общем случае адрес должен быть прочитан из BIOS data area.
#define CURSOR_PORT (0x3D4)
#define VIDEO_WIDTH (80) // Ширина текстового экрана
// Функция переводит курсор на строку strnum (0 – самая верхняя) в позицию pos на этой строке (0 – самое левое положение).
void cursor_moveto(unsigned int strnum, unsigned int pos)
{
	unsigned short new_pos = (strnum * VIDEO_WIDTH) + pos;
	outb(CURSOR_PORT, 0x0F);
	outb(CURSOR_PORT + 1, (unsigned char)(new_pos & 0xFF));

	outb(CURSOR_PORT, 0x0E);
	outb(CURSOR_PORT + 1, (unsigned char)((new_pos >> 8) & 0xFF));
}


extern "C" int kmain()
{

	// Вывод строки
	//out_str(0x07, hello, 0);
	//out_str(0x07, g_test, 1);
	//intr_init();
	//

	const char* a = "Welcome to SolverOS!";
	const char* b = "                                                                                ";
	const char* c = "#";


	int color_of_text = *((short int*)(0x470));
	cursor_position_x = 2;
	cursor_position_y = 1;
	for (int i = 1; i < 25; i++)
		out_str(0x07, b, i);

	if (color_of_text == 1)
	{
		out_str(0x0f, a, 0);
		str_color = "white";
		color_T = 0x0f;
	}
	else
		if (color_of_text == 2)
		{
			out_str(0x0e, a, 0);
			str_color = "yellow";
			color_T = 0x0e;
		}
		else
			if (color_of_text == 3)
			{
				out_str(0x0b, a, 0);
				str_color = "cian";
				color_T = 0x0b;
			}
			else
				if (color_of_text == 4)
				{
					out_str(0x0d, a, 0);
					str_color = "magenta";
					color_T = 0x0d;
				}
				else
					if (color_of_text == 5)
					{
						out_str(0x0a, a, 0);
						str_color = "green";
						color_T = 0x0a;
					}
					else
						if (color_of_text == 0)
						{
							out_str(0x08, a, 0);
							str_color = "gray";
							color_T = 0x08;
						}
	cursor_moveto(1, 2);
	out_str(color_T, c, 1);

	intr_disable();
	intr_init();
	keyb_init();
	intr_start();
	intr_enable();


	// Бесконечный цикл
	while (1)
	{
		asm("hlt");

	}
	return 0;
}







