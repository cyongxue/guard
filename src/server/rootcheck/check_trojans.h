/*
 *  check_trojans.h, Copyright (c) 2017, cyongxue@163.com
 *  All rights reserved
 *
 *  Created on: 2017年4月7日
 *      Author: yongxue@cyongxue@163.com
 */

#ifndef SRC_SERVER_ROOTCHECK_CHECK_TROJANS_H_
#define SRC_SERVER_ROOTCHECK_CHECK_TROJANS_H_

#include "defs.h"

/**
 * 目前猜测，是利用类似strings的命令来分析指定文件
 */

#ifdef SOLARIS		/* Solaris platform */
#include <sys/exechdr.h>
#elif defined Darwin || defined HPUX	/* mac platform */
/* For some reason darwin does not have that */
struct exec {
    unsigned long a_info;         /* Use macros N_MAGIC, etc for access */
    unsigned char   a_machtype;   /* machine type */
    unsigned short  a_magic;      /* magic number */
    unsigned a_text;              /* length of text, in bytes */
    unsigned a_data;              /* length of data, in bytes */
    unsigned a_bss;               /* length of uninitialized data area for file, in bytes */
    unsigned a_syms;              /* length of symbol table data in file, in bytes */
    unsigned a_entry;             /* start address */
    unsigned a_trsize;            /* length of relocation info for text, in bytes */
    unsigned a_drsize;            /* length of relocation info for data, in bytes */
};

#define OMAGIC      0407     /* Object file or impure executable */
#define NMAGIC      0410     /* Code indicating pure executable */
#define ZMAGIC      0413     /* Code indicating demand-paged executable */
#define BMAGIC      0415     /* Used by a b.out object */
#define M_OLDSUN2      0

#else				/* linux */
#include <a.out.h>
#endif

#ifndef PAGSIZ
#define       PAGSIZ          0x02000
#endif

#ifndef OLD_PAGSIZ
#define       OLD_PAGSIZ      0x00800
#endif

#ifndef N_BADMAG
#define N_BADMAG(x) (((x).a_magic)!=OMAGIC && ((x).a_magic)!=NMAGIC && ((x).a_magic)!=ZMAGIC)
#endif

#ifndef N_PAGSIZ
#define N_PAGSIZ(x) ((x).a_machtype == M_OLDSUN2? OLD_PAGSIZ : PAGSIZ)
#endif

#ifndef N_TXTOFF
#define N_TXTOFF(x) ((x).a_machtype == M_OLDSUN2 \
		? ((x).a_magic==ZMAGIC ? N_PAGSIZ(x) : sizeof (struct exec)) \
				: ((x).a_magic==ZMAGIC ? 0 : sizeof (struct exec)) )
#endif

#define STR_MINLEN			4			/* Minumum length for a string */
#define IS_STR(ch)			(isascii(ch) && (isprint(ch) || ch == '\t'))

typedef struct exec Exec;
class Strings {
	private:
		int 		_head_len;
		int 		_read_len;
		int			_hcnt;
		long		_offset;
		unsigned char 		_buf[sizeof(exec)];

		FILE*		_fp;
		std::string 	_file_name;
		std::string		_regex;
	public:
		Strings(const std::string& file_name, const std::string& regex) {
			_fp = nullptr;
			_hcnt = 0;
			_offset = 0;
			_head_len = 0;
			_read_len = -1;
		}
		~Strings() {
			if (_fp != nullptr) {
				fclose(_fp);
				_fp = nullptr;
			}
		}

		int getch();
		bool match();
};


#endif /* SRC_SERVER_ROOTCHECK_CHECK_TROJANS_H_ */
