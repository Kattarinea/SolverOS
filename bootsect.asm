use16
org 0x7C00
start:
; Инициализация адресов сегментов. Эти операции требуется не для любого BIOS, но их рекомендуется проводить.
mov ax, cs ; Сохранение адреса сегмента кода в ax
mov ds, ax ; Сохранение этого адреса как начало сегмента данных
mov ss, ax ; И сегмента стека
mov sp, start ; Сохранение адреса стека как адрес первой инструкции этого кода. Стек будет расти вверх и не перекроет код.



mov ah,0
mov al,3
int 0x10

mov dl, 0x01
mov dh, 0x00
mov cl, 0x01
mov ch, 0x00
mov al, 0x1c
mov bx, 0x1000
mov es, bx
xor bx, bx
mov ah, 0x02
int 0x13

mov ah, 0x0e
mov al, 'g'
int 0x10
mov al, 'r'
int 0x10
mov al, 'a'
int 0x10
mov al, 'y'
int 0x10
mov al, 0x0a
int 0x10
mov al,0x0d
int 0x10
mov al, 'w'
int 0x10
mov al, 'h'
int 0x10
mov al, 'i'
int 0x10
mov al, 't'
int 0x10
mov al, 'e'
int 0x10
mov al, 0x0a
int 0x10
mov al,0x0d
int 0x10
mov al, 'y'
int 0x10
mov al, 'e'
int 0x10
mov al, 'l'
int 0x10
int 0x10
mov al, 'o'
int 0x10
mov al, 'w'
int 0x10
mov al, 0x0a
int 0x10
mov al,0x0d
int 0x10
mov al, 'c'
int 0x10
mov al, 'i'
int 0x10
mov al, 'a'
int 0x10
mov al, 'n'
int 0x10
mov al, 0x0a
int 0x10
mov al,0x0d
int 0x10
mov al, 'm'
int 0x10
mov al, 'a'
int 0x10
mov al, 'g'
int 0x10
mov al, 'e'
int 0x10
mov al, 'n'
int 0x10
mov al, 't'
int 0x10
mov al, 'a'
int 0x10
mov al, 0x0a
int 0x10
mov al,0x0d
int 0x10
mov al, 'g'
int 0x10
mov al, 'r'
int 0x10
mov al, 'e'
int 0x10
int 0x10
mov al, 'n'
int 0x10


mov ch,5
mov ah,2
mov bh,0
mov dh,5
mov dl,0x0c
int 0x10

inf_loop:
mov ah, 0x00
int 0x16
cmp ah, 0x50
je plus
cmp ah, 0x48
je mines
cmp ah, 0x1c
je theend
jmp inf_loop
plus:
cmp ch,5
je loop_1
inc ch

jmp loop_1

mines:
cmp ch,0
je loop_1
dec ch

loop_1:
mov ah,2
mov bh,0
mov dh,ch
mov dl,0x0c
int 0x10
jmp inf_loop


theend:
mov [0x470],ch

; Отключение прерываний
cli
; Загрузка размера и адреса таблицы дескрипторов
lgdt [gdt_info] ; Для GNU assembler должно быть "lgdt gdt_info"
; Включение адресной линии А20
in al, 0x92
or al, 2
out 0x92, al
; Установка бита PE регистра CR0 - процессор перейдет в защищенный режим
mov eax, cr0
or al, 1
mov cr0, eax

jmp 0x8:protected_mode ; "Дальний" переход для загрузки корректной информации в cs (архитектурные особенности не позволяют этого сделать напрямую).


gdt:
; Нулевой дескриптор
db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
; Сегмент кода: base=0, size=4Gb, P=1, DPL=0, S=1(user),
; Type=1(code), Access=00A, G=1, B=32bit
db 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
; Сегмент данных: base=0, size=4Gb, P=1, DPL=0, S=1(user),
; Type=0(data), Access=0W0, G=1, B=32bit
db 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
gdt_info: ; Данные о таблице GDT (размер, положение в памяти)
dw gdt_info - gdt
; Размер таблицы (2 байта)
dw gdt, 0
; 32-битный физический адрес таблицы.



; Адрес равен адресу загрузки в случае если ядро скомпилировано в "плоский" код
use32
protected_mode:
; Здесь идут первые инструкции в защищенном режиме

; Загрузка селекторов сегментов для стека и данных в регистры
mov ax, 0x10 ; Используется дескриптор с номером 2 в GDT
mov es, ax
mov ds, ax
mov ss, ax


; Передача управления загруженному ядру
call 0x10000


times (512 - ($ - start) - 2) db 0 ; Заполнение нулями до границы 512 - 2 текущей точки
db 0x55, 0xAA ; 2 необходимых байта чтобы сектор считался загрузочным