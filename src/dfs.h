#define ERR_LOCKED		0xC3
#define ERR_BADDIR		0xCE
#define ERR_BADATTRIBUTE	0xCF
#define ERR_FILENOTFOUND	0xD6
#define ERR_CHANNEL		0xDE
#define ERR_EOF			0xDF
#define ERR_BAD_COMMAND		0xFE

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
