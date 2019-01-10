class dfs {
	struct {
		unsigned char		type = 'D';
		FILE *fp;
		unsigned char		title[12];
		unsigned char		diskcycles;
		unsigned int		totalsectors;
		unsigned char		bootoption;
		struct {
			unsigned char	name[7];
			unsigned char	dir;
			unsigned int	loadaddr;
			unsigned int	execaddr;
			unsigned int	length;
			unsigned int	startsect;
			struct {
				bool	L;
			} attrib;
		} fsp[31];
	} DFSDisc;

	void dfs::access(char *listspec, char *attributes);
	void dfs::cat(char *objspec);
	void dfs::dir(unsigned char objspec);
	void dfs::compact(char *sp, char *lp);
	void dfs::copy(char *listspec, char *objspec);
	void dfs::delete(char *objspec);
	void dfs::lib(unsigned char objspec);
	int dfs::mount(char *filename);
	void dfs::dismount(int slot);
};
