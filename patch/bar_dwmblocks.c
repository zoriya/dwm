static int dwmblockssig;
pid_t dwmblockspid = 0;


int
getdwmblockspid()
{
	char buf[16];
	FILE *fp = popen("pidof -s " STATUSBAR, "r");
	if (fgets(buf, sizeof(buf), fp));
	pid_t pid = strtoul(buf, NULL, 10);
	pclose(fp);
	dwmblockspid = pid;
	return pid != 0 ? 0 : -1;
}

void
sigdwmblocks(const Arg *arg)
{
	union sigval sv;

	if (!dwmblockssig)
		return;
	sv.sival_int = arg->i;
	if (!dwmblockspid)
		if (getdwmblockspid() == -1)
			return;

	sigqueue(dwmblockspid, SIGRTMIN+dwmblockssig, sv);
}
