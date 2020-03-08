int main()
{
	char *shellcode = "<the shellcode>";

	(*(void(*)()) shellcode)();
}
