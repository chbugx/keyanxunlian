#include "common.h"
#include "logfiles.h"
#include "log.h"


#define   _SAME_FILE_ERROR	-1
#define   _SAME_FILE_NO	0
#define   _SAME_FILE_YES	1
#define   _SAME_FILE_RETRY	2


static int	split_string(const char *str, const char *del, char **part1, char **part2)
{
	const char	*__function_name = "split_string";
	size_t		str_length = 0, part1_length = 0, part2_length = 0;
	int		ret = FAIL;

	assert(NULL != str && '\0' != *str);
	assert(NULL != del && '\0' != *del);
	assert(NULL != part1 && '\0' == *part1);	/* target 1 must be empty */
	assert(NULL != part2 && '\0' == *part2);	/* target 2 must be empty */

	 _log(LOG_LEVEL_DEBUG, "In %s() str:'%s' del:'%s'", __function_name, str, del);

	str_length = strlen(str);

	if (del < str || del >= (str + str_length - 1))
	{
		 _log(LOG_LEVEL_DEBUG, "%s() cannot proceed: delimiter is out of range", __function_name);
		goto out;
	}

	part1_length = (size_t)(del - str + 1);
	part2_length = str_length - part1_length;

	*part1 =   _malloc(*part1, part1_length + 1);
	  _strlcpy(*part1, str, part1_length + 1);

	*part2 =   _malloc(*part2, part2_length + 1);
	  _strlcpy(*part2, str + part1_length, part2_length + 1);

	ret = SUCCEED;
out:
	 _log(LOG_LEVEL_DEBUG, "End of %s():%s part1:'%s' part2:'%s'", __function_name,   _result_string(ret),
			*part1, *part2);

	return ret;
}


static int	split_filename(const char *filename, char **directory, char **format)
{
	const char	*__function_name = "split_filename";
	const char	*separator = NULL;
	  _stat_t	buf;
	int		ret = FAIL;


	if (NULL == filename || '\0' == *filename)
	{
		 _log(LOG_LEVEL_WARNING, "cannot split empty path");
		goto out;
	}


	if (NULL == (separator = strrchr(filename, (int)PATH_SEPARATOR)))
	{
		
		goto out;
	}
	if (SUCCEED != split_string(filename, separator, directory, format))
	{
		 
		goto out;
	}

	if (-1 ==   _stat(*directory, &buf))
	{
		
		  _free(*directory);
		  _free(*format);
		goto out;
	}

	if (0 == S_ISDIR(buf.st_mode))
	{
		 _log(LOG_LEVEL_WARNING, "cannot proceed: directory '%s' is a file", *directory);
		  _free(*directory);
		  _free(*format);
		goto out;
	}


	ret = SUCCEED;
out:
	 _log(LOG_LEVEL_DEBUG, "End of %s():%s directory:'%s' format:'%s'", __function_name,   _result_string(ret),
			*directory, *format);

	return ret;
}


static int	file_start_md5(int f, int length, md5_byte_t *md5buf, const char *filename)
{
	md5_state_t	state;
	char		buf[MAX_LEN_MD5];

	if (MAX_LEN_MD5 < length)
		return FAIL;

	if ((  _offset_t)-1 ==   _lseek(f, 0, SEEK_SET))
	{
		 
		return FAIL;
	}

	if (length != (int)read(f, buf, (size_t)length))
	{
		 
		return FAIL;
	}

	md5_init(&state);
	md5_append(&state, (const md5_byte_t *)buf, length);
	md5_finish(&state, md5buf);

	return SUCCEED;
}



static int	is_same_file(const struct st_logfile *old, const struct st_logfile *new, int use_ino)
{
	int	ret =   _SAME_FILE_NO;

	if (1 == use_ino || 2 == use_ino)
	{
		if (old->ino_lo != new->ino_lo || old->dev != new->dev)
		{
			/* File's inode and device id cannot differ. */
			goto out;
		}
	}

	if (2 == use_ino && old->ino_hi != new->ino_hi)
	{
		/* File's inode (older 64-bits) cannot differ. */
		goto out;
	}

	if (old->mtime > new->mtime)
	{
		/* File's mtime cannot decrease unless manipulated. */
		goto out;
	}

	if (old->size > new->size)
	{
		
		goto out;
	}

	if (old->md5size > new->md5size)
	{
		
		goto out;
	}

	if (old->md5size == new->md5size)
	{
		if (0 != memcmp(old->md5buf, new->md5buf, sizeof(new->md5buf)))
		{
			
			goto out;
		}
	}
	else
	{
		if (0 < old->md5size)
		{
			
			int		f;
			md5_byte_t	md5tmp[MD5_DIGEST_SIZE];

			if (-1 == (f =   _open(new->filename, O_RDONLY)))
			{
				ret =   _SAME_FILE_ERROR;
				goto out;
			}

			if (SUCCEED == file_start_md5(f, old->md5size, md5tmp, new->filename))
			{
				ret = (0 == memcmp(old->md5buf, &md5tmp, sizeof(md5tmp))) ?   _SAME_FILE_YES :
						  _SAME_FILE_NO;
			}
			else
				ret =   _SAME_FILE_ERROR;

			if (0 != close(f))
			{
				ret =   _SAME_FILE_ERROR;
			}

			goto out;
		}
	}

	ret =   _SAME_FILE_YES;
out:
	return ret;
}

/******************************************************************************
 *                                                                            *
 * Function: setup_old2new                                                    *
 *                                                                            *
 * Purpose: fill an array of possible mappings from the old log files to the  *
 *          new log files.                                                    *
 *                                                                            *
 * Parameters:                                                                *
 *          old2new - [IN] two dimensional array of possible mappings         *
 *          old     - [IN] old file list                                      *
 *          num_old - [IN] number of elements in the old file list            *
 *          new     - [IN] new file list                                      *
 *          num_new - [IN] number of elements in the new file list            *
 *          use_ino - [IN] how to use inodes in is_same_file()                *
 *                                                                            *
 * Return value: SUCCEED or FAIL                                              *
 *                                                                            *
 * Comments:                                                                  *
 *    The array is filled with '0' and '1' which mean:                        *
 *       old2new[i][j] = '0' - the i-th old file IS NOT the j-th new file     *
 *       old2new[i][j] = '1' - the i-th old file COULD BE the j-th new file   *
 *                                                                            *
 ******************************************************************************/
static int	setup_old2new(char *old2new, struct st_logfile *old, int num_old,
		const struct st_logfile *new, int num_new, int use_ino)
{
	int	i, j, rc;
	char	*p = old2new;

	for (i = 0; i < num_old; i++)
	{
		for (j = 0; j < num_new; j++)
		{
			rc = is_same_file(old + i, new + j, use_ino);

			switch (rc)
			{
				case   _SAME_FILE_NO:
					p[j] = '0';
					break;
				case   _SAME_FILE_YES:
					if (1 == old[i].retry)
					{
						
						old[i].retry = 0;
					}
					p[j] = '1';
					break;
				case   _SAME_FILE_RETRY:
					old[i].retry = 1;
					/* break; is not missing here */
				case   _SAME_FILE_ERROR:
					return FAIL;
			}

		}
		p += (size_t)num_new;
	}
	return SUCCEED;
}



static int	is_uniq_row(const char *arr, int n_cols, int row)
{
	int		i, ones = 0, ret = -1;
	const char	*p;

	p = arr + row * n_cols;			/* point to the first element of the 'row' */

	for (i = 0; i < n_cols; i++)
	{
		if ('1' == *p++)
		{
			if (2 == ++ones)
			{
				ret = -1;	/* non-unique mapping in the row */
				break;
			}

			ret = i;
		}
	}

	return ret;
}


static int	is_uniq_col(const char *arr, int n_rows, int n_cols, int col)
{
	int		i, ones = 0, ret = -1;
	const char	*p;

	p = arr + col;				/* point to the top element of the 'col' */

	for (i = 0; i < n_rows; i++)
	{
		if ('1' == *p)
		{
			if (2 == ++ones)
			{
				ret = -1;	/* non-unique mapping in the column */
				break;
			}

			ret = i;
		}
		p += n_cols;
	}

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Function: resolve_old2new                                                  *
 *                                                                            *
 * Purpose: resolve non-unique mappings                                       *
 *                                                                            *
 * Parameters:                                                                *
 *          old2new - [IN] two dimensional array of possible mappings         *
 *          num_old - [IN] number of elements in the old file list            *
 *          num_new - [IN] number of elements in the new file list            *
 *                                                                            *
 ******************************************************************************/
static void	resolve_old2new(char *old2new, int num_old, int num_new)
{
	int	i, j, ones;
	char	*p, *protected_rows = NULL, *protected_cols = NULL;

	/* Is there 1:1 mapping in both directions between files in the old and the new list ? */
	/* In this case every row and column has not more than one element '1'. */
	/* This is expected on UNIX (using inode numbers) and MS Windows (using FileID on NTFS, ReFS) */

	p = old2new;

	for (i = 0; i < num_old; i++)		/* loop over rows (old files) */
	{
		ones = 0;

		for (j = 0; j < num_new; j++)	/* loop over columns (new files) */
		{
			if ('1' == *p++)
			{
				if (2 == ++ones)
					goto non_unique;
			}
		}
	}

	for (i = 0; i < num_new; i++)		/* loop over columns */
	{
		p = old2new + i;
		ones = 0;

		for (j = 0; j < num_old; j++)	/* loop over rows */
		{
			if ('1' == *p)
			{
				if (2 == ++ones)
					goto non_unique;
			}
			p += num_new;
		}
	}

	return;
non_unique:
	/* This is expected on MS Windows using FAT32 and other file systems where inodes or file indexes */
	/* are either not preserved if a file is renamed or are not applicable. */

	 _log(LOG_LEVEL_DEBUG, "resolve_old2new(): non-unique mapping");

	/* protect unique mappings from further modifications */

	protected_rows =   _calloc(protected_rows, (size_t)num_old, sizeof(char));
	protected_cols =   _calloc(protected_cols, (size_t)num_new, sizeof(char));

	for (i = 0; i < num_old; i++)
	{
		int	c;

		if (-1 != (c = is_uniq_row(old2new, num_new, i)) && -1 != is_uniq_col(old2new, num_old, num_new, c))
		{
			protected_rows[i] = '1';
			protected_cols[c] = '1';
		}
	}

	/* resolve the remaining non-unique mappings - turn them into unique ones */

	if (num_old <= num_new)				/* square or wide array */
	{
		

		for (i = 0; i < num_old; i++)		/* loop over rows from top-left corner */
		{
			if ('1' == protected_rows[i])
				continue;

			p = old2new + i * num_new;	/* the first element of the current row */

			for (j = 0; j < num_new; j++)
			{
				if ('1' == p[j] && '1' != protected_cols[j])
				{
					cross_out(old2new, num_old, num_new, i, j, protected_rows, protected_cols);
					break;
				}
			}
		}
	}
	else	/* tall array */
	{
		

		for (i = num_old - 1; i >= 0; i--)	/* loop over rows from bottom-right corner */
		{
			if ('1' == protected_rows[i])
				continue;

			p = old2new + i * num_new;	/* the first element of the current row */

			for (j = num_new - 1; j >= 0; j--)
			{
				if ('1' == p[j] && '1' != protected_cols[j])
				{
					cross_out(old2new, num_old, num_new, i, j, protected_rows, protected_cols);
					break;
				}
			}
		}
	}

	  _free(protected_cols);
	  _free(protected_rows);
}

/******************************************************************************
 *                                                                            *
 * Function: find_old2new                                                     *
 *                                                                            *
 * Purpose: find a mapping from old to new file                               *
 *                                                                            *
 * Parameters:                                                                *
 *          old2new - [IN] two dimensional array of possible mappings         *
 *          num_new - [IN] number of elements in the new file list            *
 *          i_old   - [IN] index of the old file                              *
 *                                                                            *
 * Return value: index of the new file or                                     *
 *               -1 if no mapping was found                                   *
 *                                                                            *
 ******************************************************************************/
static int	find_old2new(char *old2new, int num_new, int i_old)
{
	int	i;
	char	*p;

	p = old2new + i_old * num_new;

	for (i = 0; i < num_new; i++)		/* loop over columns (new files) on i_old-th row */
	{
		if ('1' == *p++)
			return i;
	}

	return -1;
}


static void add_logfile(struct st_logfile **logfiles, int *logfiles_alloc, int *logfiles_num, const char *filename,
		  _stat_t *st)
{
	const char	*__function_name = "add_logfile";
	int		i = 0, cmp = 0;

	 _log(LOG_LEVEL_DEBUG, "In %s() filename:'%s' mtime:%d size:"   _FS_UI64, __function_name, filename,
			(int)st->st_mtime, (  _uint64_t)st->st_size);

	/* must be done in any case */
	if (*logfiles_alloc == *logfiles_num)
	{
		*logfiles_alloc += 64;
		*logfiles =   _realloc(*logfiles, (size_t)*logfiles_alloc * sizeof(struct st_logfile));

		 _log(LOG_LEVEL_DEBUG, "%s() logfiles:%p logfiles_alloc:%d",
				__function_name, *logfiles, *logfiles_alloc);
	}

	

	for (; i < *logfiles_num; i++)
	{
		if (st->st_mtime > (*logfiles)[i].mtime)
			continue;	/* (1) sort by ascending mtime */

		if (st->st_mtime == (*logfiles)[i].mtime)
		{
			if (0 > (cmp = strcmp(filename, (*logfiles)[i].filename)))
				continue;	/* (2) sort by descending name */

			if (0 == cmp)
			{
				/* the file already exists, quite impossible branch */
				 _log(LOG_LEVEL_WARNING, "%s() file '%s' already added", __function_name,
						filename);
				goto out;
			}

			/* filename is smaller, must insert here */
		}

		/* the place is found, move all from the position forward by one struct */
		break;
	}

	if (*logfiles_num > i)
	{
		/* free a gap for inserting the new element */
		memmove((void *)&(*logfiles)[i + 1], (const void *)&(*logfiles)[i],
				(size_t)(*logfiles_num - i) * sizeof(struct st_logfile));
	}

	(*logfiles)[i].filename =   _strdup(NULL, filename);
	(*logfiles)[i].mtime = st->st_mtime;
	(*logfiles)[i].md5size = -1;
	(*logfiles)[i].seq = 0;
	(*logfiles)[i].incomplete = 0;
#ifndef _WINDOWS
	(*logfiles)[i].dev = (  _uint64_t)st->st_dev;
	(*logfiles)[i].ino_lo = (  _uint64_t)st->st_ino;
	(*logfiles)[i].ino_hi = 0;
#endif
	(*logfiles)[i].size = (  _uint64_t)st->st_size;
	(*logfiles)[i].processed_size = 0;
	(*logfiles)[i].retry = 0;

	++(*logfiles_num);
out:
	 _log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
}

/******************************************************************************
 *                                                                            *
 * Function: destroy_logfile_list                                             *
 *                                                                            *
 * Purpose: release resources allocated to a logfile list                     *
 *                                                                            *
 * Parameters:                                                                *
 *     logfiles       - [IN/OUT] pointer to the list of logfiles              *
 *     logfiles_alloc - [IN/OUT] number of logfiles memory was allocated for  *
 *     logfiles_num   - [IN/OUT] number of already inserted logfiles          *
 *                                                                            *
 ******************************************************************************/
static void	destroy_logfile_list(struct st_logfile **logfiles, int *logfiles_alloc, int *logfiles_num)
{
	int	i;

	for (i = 0; i < *logfiles_num; i++)
		  _free((*logfiles)[i].filename);

	*logfiles_num = 0;
	*logfiles_alloc = 0;
	  _free(*logfiles);
}

/******************************************************************************
 *                                                                            *
 * Function: pick_logfile                                                     *
 *                                                                            *
 * Purpose: checks if the specified file meets requirements and adds it to    *
 *          the logfile list                                                  *
 *                                                                            *
 * Parameters:                                                                *
 *     directory      - [IN] directory where the logfiles reside              *
 *     filename       - [IN] name of the logfile (without path)               *
 *     mtime          - [IN] selection criterion "logfile modification time"  *
 *                      The logfile will be selected if modified not before   *
 *                      'mtime'.                                              *
 *     re             - [IN] selection criterion "regexp describing filename  *
 *                      pattern"                                              *
 *     logfiles       - [IN/OUT] pointer to the list of logfiles              *
 *     logfiles_alloc - [IN/OUT] number of logfiles memory was allocated for  *
 *     logfiles_num   - [IN/OUT] number of already inserted logfiles          *
 *                                                                            *
 * Comments: This is a helper function for pick_logfiles()                    *
 *                                                                            *
 ******************************************************************************/
static void	pick_logfile(const char *directory, const char *filename, int mtime, const regex_t *re,
		struct st_logfile **logfiles, int *logfiles_alloc, int *logfiles_num)
{
	char		*logfile_candidate = NULL;
	  _stat_t	file_buf;

	logfile_candidate =   _dsprintf(logfile_candidate, "%s%s", directory, filename);

	if (0 ==   _stat(logfile_candidate, &file_buf))
	{
		if (S_ISREG(file_buf.st_mode) &&
				mtime <= file_buf.st_mtime &&
				0 == regexec(re, filename, (size_t)0, NULL, 0))
		{
			add_logfile(logfiles, logfiles_alloc, logfiles_num, logfile_candidate, &file_buf);
		}
	}
	else
		 _log(LOG_LEVEL_DEBUG, "cannot process entry '%s': %s", logfile_candidate,   _strerror(errno));

	  _free(logfile_candidate);
}


static int	pick_logfiles(const char *directory, int mtime, const regex_t *re, int *use_ino,
		struct st_logfile **logfiles, int *logfiles_alloc, int *logfiles_num)
{
#ifdef _WINDOWS
	int			ret = FAIL;
	char			*find_path = NULL, *file_name_utf8;
	wchar_t			*find_wpath = NULL;
	intptr_t		find_handle;
	struct _wfinddata_t	find_data;

	/* "open" Windows directory */
	find_path =   _dsprintf(find_path, "%s*", directory);
	find_wpath =   _utf8_to_unicode(find_path);

	if (-1 == (find_handle = _wfindfirst(find_wpath, &find_data)))
	{
		 _log(LOG_LEVEL_WARNING, "cannot open directory \"%s\" for reading: %s", directory,
				  _strerror(errno));
		  _free(find_wpath);
		  _free(find_path);
		return FAIL;
	}

	if (SUCCEED != set_use_ino_by_fs_type(find_path, use_ino))
		goto clean;

	do
	{
		file_name_utf8 =   _unicode_to_utf8(find_data.name);
		pick_logfile(directory, file_name_utf8, mtime, re, logfiles, logfiles_alloc, logfiles_num);
		  _free(file_name_utf8);
	}
	while (0 == _wfindnext(find_handle, &find_data));

	ret = SUCCEED;
clean:
	if (-1 == _findclose(find_handle))
	{
		 _log(LOG_LEVEL_WARNING, "cannot close the find directory handle for '%s': %s", find_path,
				  _strerror(errno));
		ret = FAIL;
	}

	  _free(find_wpath);
	  _free(find_path);

	return ret;
#else
	DIR		*dir = NULL;
	struct dirent	*d_ent = NULL;

	if (NULL == (dir = opendir(directory)))
	{
		 _log(LOG_LEVEL_WARNING, "cannot open directory \"%s\" for reading: %s", directory,
				  _strerror(errno));
		return FAIL;
	}

	/* on UNIX file systems we always assume that inodes can be used to identify files */
	*use_ino = 1;

	while (NULL != (d_ent = readdir(dir)))
	{
		pick_logfile(directory, d_ent->d_name, mtime, re, logfiles, logfiles_alloc, logfiles_num);
	}

	if (-1 == closedir(dir))
	{
		 _log(LOG_LEVEL_WARNING, "cannot close directory '%s': %s", directory,   _strerror(errno));
		return FAIL;
	}

	return SUCCEED;
#endif
}


static int	make_logfile_list(int is_logrt, const char *filename, const int *mtime, struct st_logfile **logfiles,
		int *logfiles_alloc, int *logfiles_num, int *use_ino)
{
	int		ret = SUCCEED, i;
	  _stat_t	file_buf;

	if (0 == is_logrt)	/* log[] item */
	{
		if (0 !=   _stat(filename, &file_buf))
		{
			 _log(LOG_LEVEL_WARNING, "cannot stat '%s': %s", filename,   _strerror(errno));
			ret = FAIL;
			goto clean;
		}

		if (!S_ISREG(file_buf.st_mode))
		{
			 _log(LOG_LEVEL_WARNING, "'%s' is not a regular file, it cannot be used in log[] item",
					filename);
			ret = FAIL;
			goto clean;
		}

		add_logfile(logfiles, logfiles_alloc, logfiles_num, filename, &file_buf);
#ifdef _WINDOWS
		if (SUCCEED != set_use_ino_by_fs_type(filename, use_ino))
		{
			ret = FAIL;
			goto clean;
		}
#else
		/* on UNIX file systems we always assume that inodes can be used to identify files */
		*use_ino = 1;
#endif
	}
	else	/* logrt[] item */
	{
		char			*directory = NULL, *format = NULL;
		int			reg_error;
		regex_t			re;

		/* split a filename into directory and file mask (regular expression) parts */
		if (SUCCEED != split_filename(filename, &directory, &format))
		{
			 _log(LOG_LEVEL_WARNING, "filename '%s' does not contain a valid directory and/or format",
					filename);
			ret = FAIL;
			goto clean;
		}

		if (0 != (reg_error = regcomp(&re, format, REG_EXTENDED | REG_NEWLINE | REG_NOSUB)))
		{
			char	err_buf[MAX_STRING_LEN];

			regerror(reg_error, &re, err_buf, sizeof(err_buf));
			 _log(LOG_LEVEL_WARNING, "Cannot compile a regexp describing filename pattern '%s' for "
					"a logrt[] item. Error: %s", format, err_buf);
			ret = FAIL;
#ifdef _WINDOWS
			/* the Windows gnuregex implementation does not correctly clean up */
			/* allocated memory after regcomp() failure                        */
			regfree(&re);
#endif
			goto clean1;
		}

		if (SUCCEED != pick_logfiles(directory, *mtime, &re, use_ino, logfiles, logfiles_alloc, logfiles_num))
		{
			ret = FAIL;
			goto clean2;
		}

		if (0 == *logfiles_num)
		{
			/* Do not make a logrt[] item NOTSUPPORTED if there are no matching log files or they are not */
			/* accessible (can happen during a rotation), just log the problem. */
#ifdef _WINDOWS
			 _log(LOG_LEVEL_WARNING, "there are no files matching \"%s\" in \"%s\" or insufficient "
					"access rights", format, directory);
#else
			if (0 != access(directory, X_OK))
			{
				 _log(LOG_LEVEL_WARNING, "insufficient access rights (no \"execute\" permission) "
						"to directory \"%s\": %s", directory,   _strerror(errno));
			}
			else
			{
				 _log(LOG_LEVEL_WARNING, "there are no files matching \"%s\" in \"%s\"", format,
						directory);
			}
#endif
		}
clean2:
		regfree(&re);
clean1:
		  _free(directory);
		  _free(format);

		if (FAIL == ret)
			goto clean;
	}

	/* Fill in MD5 sums and file indexes in the logfile list. */
	/* These operations require opening of file, therefore we group them together. */
	for (i = 0; i < *logfiles_num; i++)
	{
		int			f;
		struct st_logfile	*p;

		p = *logfiles + i;

		if (-1 == (f =   _open(p->filename, O_RDONLY)))
		{
			 _log(LOG_LEVEL_WARNING, "cannot open \"%s\"': %s", p->filename,   _strerror(errno));
			ret = FAIL;
			break;
		}

		p->md5size = (  _uint64_t)MAX_LEN_MD5 > p->size ? (int)p->size : MAX_LEN_MD5;

		if (SUCCEED != file_start_md5(f, p->md5size, p->md5buf, p->filename))
		{
			ret = FAIL;
			goto clean3;
		}
#ifdef _WINDOWS
		if (SUCCEED != file_id(f, *use_ino, &p->dev, &p->ino_lo, &p->ino_hi, p->filename))
			ret = FAIL;
#endif	/*_WINDOWS*/
clean3:
		if (0 != close(f))
		{
			 _log(LOG_LEVEL_WARNING, "cannot close file '%s': %s", p->filename,   _strerror(errno));
			ret = FAIL;
			break;
		}
	}
clean:
	if (FAIL == ret && NULL != *logfiles)
		destroy_logfile_list(logfiles, logfiles_alloc, logfiles_num);

	return	ret;
}

static char	*buf_find_newline(char *p, char **p_next, const char *p_end, const char *cr, const char *lf,
		size_t szbyte)
{
	if (1 == szbyte)	/* single-byte character set */
	{
		for (; p < p_end; p++)
		{
			if (0xd < *p || 0xa > *p)
				continue;

			if (0xa == *p)  /* LF (Unix) */
			{
				*p_next = p + 1;
				return p;
			}

			if (0xd == *p)	/* CR (Mac) */
			{
				if (p < p_end - 1 && 0xa == *(p + 1))   /* CR+LF (Windows) */
				{
					*p_next = p + 2;
					return p;
				}

				*p_next = p + 1;
				return p;
			}
		}
		return (char *)NULL;
	}
	else
	{
		while (p <= p_end - szbyte)
		{
			if (0 == memcmp(p, lf, szbyte))		/* LF (Unix) */
			{
				*p_next = p + szbyte;
				return p;
			}

			if (0 == memcmp(p, cr, szbyte))		/* CR (Mac) */
			{
				if (p <= p_end - szbyte - szbyte && 0 == memcmp(p + szbyte, lf, szbyte))
				{
					/* CR+LF (Windows) */
					*p_next = p + szbyte + szbyte;
					return p;
				}

				*p_next = p + szbyte;
				return p;
			}

			p += szbyte;
		}
		return (char *)NULL;
	}
}

static int	  _read2(int fd,   _uint64_t *lastlogsize, int *mtime, int *big_rec, int *incomplete,
		const char *encoding,   _vector_ptr_t *regexps, const char *pattern, const char *output_template,
		int *p_count, int *s_count,   _process_value_func_t process_value, const char *server,
		unsigned short port, const char *hostname, const char *key)
{
	  _THREAD_LOCAL static char	*buf = NULL;

	int				ret, nbytes;
	const char			*cr, *lf, *p_end;
	char				*p_start, *p, *p_nl, *p_next, *item_value = NULL;
	size_t				szbyte;
	  _offset_t			offset;
	int				send_err;
	  _uint64_t			lastlogsize1;

#define BUF_SIZE	(256 *   _KIBIBYTE)	/* The longest encodings use 4-bytes for every character. To send */
						/* up to 64 k characters to the   server a 256 kB buffer might */
						/* be required. */
	if (NULL == buf)
	{
		buf =   _malloc(buf, (size_t)(BUF_SIZE + 1));
	}

	find_cr_lf_szbyte(encoding, &cr, &lf, &szbyte);

	for (;;)
	{
		if (0 >= *p_count || 0 >= *s_count)
		{
			/* limit on number of processed or sent-to-server lines reached */
			ret = SUCCEED;
			goto out;
		}

		if ((  _offset_t)-1 == (offset =   _lseek(fd, 0, SEEK_CUR)))
		{
			*big_rec = 0;
			ret = FAIL;
			goto out;
		}

		nbytes = (int)read(fd, buf, (size_t)BUF_SIZE);

		if (-1 == nbytes)
		{
			/* error on read */
			*big_rec = 0;
			ret = FAIL;
			goto out;
		}

		if (0 == nbytes)
		{
			/* end of file reached */
			ret = SUCCEED;
			goto out;
		}

		p_start = buf;			/* beginning of current line */
		p = buf;			/* current byte */
		p_end = buf + (size_t)nbytes;	/* no data from this position */

		if (NULL == (p_nl = buf_find_newline(p, &p_next, p_end, cr, lf, szbyte)))
		{
			if (p_end > p)
				*incomplete = 1;

			if (BUF_SIZE > nbytes)
			{
				/* Buffer is not full (no more data available) and there is no "newline" in it. */
				/* Do not analyze it now, keep the same position in the file and wait the next check, */
				/* maybe more data will come. */

				*lastlogsize = (  _uint64_t)offset;
				ret = SUCCEED;
				goto out;
			}
			else
			{
				/* buffer is full and there is no "newline" in it */

				if (0 == *big_rec)
				{
					/* It is the first, beginning part of a long record. Match it against the */
					/* regexp now (our buffer length corresponds to what we can save in the */
					/* database). */

					char	*value = NULL;

					buf[BUF_SIZE] = '\0';

					if ('\0' != *encoding)
						value = convert_to_utf8(buf, (size_t)BUF_SIZE, encoding);
					else
						value = buf;

					 _log(LOG_LEVEL_WARNING, "Logfile contains a large record: \"%.64s\""
							" (showing only the first 64 characters). Only the first 256 kB"
							" will be analyzed, the rest will be ignored while   agent"
							" is running.", value);

					lastlogsize1 = (size_t)offset + (size_t)nbytes;
					send_err = SUCCEED;

					if (SUCCEED == regexp_sub_ex(regexps, value, pattern,   _CASE_SENSITIVE,
							output_template, &item_value))
					{
						send_err = process_value(server, port, hostname, key, item_value,
								&lastlogsize1, mtime, NULL, NULL, NULL, NULL, 1);

						  _free(item_value);

						if (SUCCEED == send_err)
							(*s_count)--;
					}
					(*p_count)--;

					if (SUCCEED == send_err)
					{
						*lastlogsize = lastlogsize1;
						*big_rec = 1;		/* ignore the rest of this record */
					}

					if ('\0' != *encoding)
						  _free(value);
				}
				else
				{
					/* It is a middle part of a long record. Ignore it. We have already */
					/* checked the first part against the regexp. */
					*lastlogsize = (size_t)offset + (size_t)nbytes;
				}
			}
		}
		else
		{
			/* the "newline" was found, so there is at least one complete record */
			/* (or trailing part of a large record) in the buffer */
			*incomplete = 0;

			for (;;)
			{
				if (0 >= *p_count || 0 >= *s_count)
				{
					/* limit on number of processed or sent-to-server lines reached */
					ret = SUCCEED;
					goto out;
				}

				if (0 == *big_rec)
				{
					char	*value = NULL;

					*p_nl = '\0';

					if ('\0' != *encoding)
						value = convert_to_utf8(p_start, (size_t)(p_nl - p_start), encoding);
					else
						value = p_start;

					lastlogsize1 = (size_t)offset + (size_t)(p_next - buf);
					send_err = SUCCEED;

					if (SUCCEED == regexp_sub_ex(regexps, value, pattern,   _CASE_SENSITIVE,
							output_template, &item_value))
					{
						send_err = process_value(server, port, hostname, key, item_value,
								&lastlogsize1, mtime, NULL, NULL, NULL, NULL, 1);

						  _free(item_value);

						if (SUCCEED == send_err)
							(*s_count)--;
					}
					(*p_count)--;

					if (SUCCEED == send_err)
						*lastlogsize = lastlogsize1;

					if ('\0' != *encoding)
						  _free(value);
				}
				else
				{
					/* skip the trailing part of a long record */
					*lastlogsize = (size_t)offset + (size_t)(p_next - buf);
					*big_rec = 0;
				}

				/* move to the next record in the buffer */
				p_start = p_next;
				p = p_next;

				if (NULL == (p_nl = buf_find_newline(p, &p_next, p_end, cr, lf, szbyte)))
				{
					/* There are no complete records in the buffer. */
					/* Try to read more data from this position if available. */
					if (p_end > p)
						*incomplete = 1;

					if ((  _offset_t)-1 ==   _lseek(fd, *lastlogsize, SEEK_SET))
					{
						ret = FAIL;
						goto out;
					}
					else
						break;
				}
				else
					*incomplete = 0;
			}
		}
	}
out:
	return ret;

#undef BUF_SIZE
}

/******************************************************************************
 *                                                                            *
 * Function: process_log                                                      *
 *                                                                            *
 * Purpose: Match new records in logfile with regexp, transmit matching       *
 *          records to   server                                          *
 *                                                                            *
 * Parameters:                                                                *
 *     filename        - [IN] logfile name                                    *
 *     lastlogsize     - [IN/OUT] offset from the beginning of the file       *
 *     mtime           - [IN] file modification time for reporting to server  *
 *     skip_old_data   - [IN/OUT] start from the beginning of the file or     *
 *                       jump to the end                                      *
 *     big_rec         - [IN/OUT] state variable to remember whether a long   *
 *                       record is being processed                            *
 *     incomplete      - [OUT] 0 - the last record ended with a newline,      *
 *                       1 - there was no newline at the end of the last      *
 *                       record.                                              *
 *     encoding        - [IN] text string describing encoding.                *
 *                         The following encodings are recognized:            *
 *                           "UNICODE"                                        *
 *                           "UNICODEBIG"                                     *
 *                           "UNICODEFFFE"                                    *
 *                           "UNICODELITTLE"                                  *
 *                           "UTF-16"   "UTF16"                               *
 *                           "UTF-16BE" "UTF16BE"                             *
 *                           "UTF-16LE" "UTF16LE"                             *
 *                           "UTF-32"   "UTF32"                               *
 *                           "UTF-32BE" "UTF32BE"                             *
 *                           "UTF-32LE" "UTF32LE".                            *
 *                           "" (empty string) means a single-byte character  *
 *                           set (e.g. ASCII).                                *
 *     regexps         - [IN] array of regexps                                *
 *     pattern         - [IN] pattern to match                                *
 *     output_template - [IN] output formatting template                      *
 *     p_count         - [IN/OUT] limit of records to be processed            *
 *     s_count         - [IN/OUT] limit of records to be sent to server       *
 *     process_value   - [IN] pointer to function process_value()             *
 *     server          - [IN] server to send data to                          *
 *     port            - [IN] port to send data to                            *
 *     hostname        - [IN] hostname the data comes from                    *
 *     key             - [IN] item key the data belongs to                    *
 *                                                                            *
 * Return value: returns SUCCEED on successful reading,                       *
 *               FAIL on other cases                                          *
 *                                                                            *
 * Author: Eugene Grigorjev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *           This function does not deal with log file rotation.              *
 *                                                                            *
 ******************************************************************************/
static int	process_log(char *filename,   _uint64_t *lastlogsize, int *mtime, unsigned char *skip_old_data,
		int *big_rec, int *incomplete, const char *encoding,   _vector_ptr_t *regexps, const char *pattern,
		const char *output_template, int *p_count, int *s_count,   _process_value_func_t process_value,
		const char *server, unsigned short port, const char *hostname, const char *key)
{
	const char	*__function_name = "process_log";

	int		f, ret = FAIL;
	  _stat_t	buf;
	  _uint64_t	l_size;

	 _log(LOG_LEVEL_DEBUG, "In %s() filename:'%s' lastlogsize:"   _FS_UI64 " mtime:%d",
			__function_name, filename, *lastlogsize, NULL != mtime ? *mtime : 0);

	if (0 !=   _stat(filename, &buf))
	{
		 _log(LOG_LEVEL_WARNING, "cannot stat '%s': %s", filename,   _strerror(errno));
		goto out;
	}

	if (NULL != mtime)
		*mtime = (int)buf.st_mtime;

	if ((  _uint64_t)buf.st_size == *lastlogsize)
	{
		/* The file size has not changed. Nothing to do. Here we do not deal with a case of changing */
		/* a logfile's content while keeping the same length. */
		ret = SUCCEED;
		goto out;
	}

	if (-1 == (f =   _open(filename, O_RDONLY)))
	{
		 _log(LOG_LEVEL_WARNING, "cannot open '%s': %s", filename,   _strerror(errno));
		goto out;
	}

	l_size = *lastlogsize;

	if (1 == *skip_old_data)
	{
		l_size = (  _uint64_t)buf.st_size;
		 _log(LOG_LEVEL_DEBUG, "skipping old data in filename:'%s' to lastlogsize:"   _FS_UI64,
				filename, l_size);
	}

	if ((  _uint64_t)buf.st_size < l_size)		/* handle file truncation */
		l_size = 0;

	if ((  _offset_t)-1 !=   _lseek(f, l_size, SEEK_SET))
	{
		*lastlogsize = l_size;
		*skip_old_data = 0;

		ret =   _read2(f, lastlogsize, mtime, big_rec, incomplete, encoding, regexps, pattern, output_template,
				p_count, s_count, process_value, server, port, hostname, key);
	}
	else
	{
		 _log(LOG_LEVEL_WARNING, "cannot set position to "   _FS_UI64 " for '%s': %s",
				l_size, filename,   _strerror(errno));
	}

	if (0 != close(f))
	{
		 _log(LOG_LEVEL_WARNING, "cannot close file '%s': %s", filename,   _strerror(errno));
		ret = FAIL;
	}
out:
	 _log(LOG_LEVEL_DEBUG, "End of %s() filename:'%s' lastlogsize:"   _FS_UI64 " mtime:%d ret:%s",
			__function_name, filename, *lastlogsize, NULL != mtime ? *mtime : 0,   _result_string(ret));

	return ret;
}


int	process_logrt(int is_logrt, char *filename,   _uint64_t *lastlogsize, int *mtime, unsigned char *skip_old_data,
		int *big_rec, int *use_ino, int *error_count, struct st_logfile **logfiles_old, int *logfiles_num_old,
		const char *encoding,   _vector_ptr_t *regexps, const char *pattern, const char *output_template,
		int *p_count, int *s_count,   _process_value_func_t process_value, const char *server,
		unsigned short port, const char *hostname, const char *key)
{
	const char		*__function_name = "process_logrt";
	int			i, j, start_idx, ret = FAIL, logfiles_num = 0, logfiles_alloc = 0, seq = 1,
				max_old_seq = 0, old_last, from_first_file = 1;
	char			*old2new = NULL;
	struct st_logfile	*logfiles = NULL;
	time_t			now;

	 _log(LOG_LEVEL_DEBUG, "In %s() is_logrt:%d filename:'%s' lastlogsize:"   _FS_UI64 " mtime:%d "
			"error_count:%d", __function_name, is_logrt, filename, *lastlogsize, *mtime, *error_count);

	/* Minimize data loss if the system clock has been set back in time. */
	/* Setting the clock ahead of time is harmless in our case. */
	if (*mtime > (now = time(NULL)))
	{
		int	old_mtime;

		old_mtime = *mtime;
		*mtime = (int)now;

		 _log(LOG_LEVEL_WARNING, "System clock has been set back in time. Setting agent mtime %d "
				"seconds back.", (int)(old_mtime - now));
	}

	if (SUCCEED != make_logfile_list(is_logrt, filename, mtime, &logfiles, &logfiles_alloc, &logfiles_num, use_ino))
	{
		/* an error occurred or a file was not accessible for a log[] item */
		(*error_count)++;
		ret = SUCCEED;
		goto out;
	}

	if (0 == logfiles_num)
	{
		/* there were no files for a logrt[] item to analyze */
		ret = SUCCEED;
		goto out;
	}

	start_idx = (1 == *skip_old_data ? logfiles_num - 1 : 0);

	/* mark files to be skipped as processed (in case of 'skip_old_data' was set) */
	for (i = 0; i < start_idx; i++)
	{
		logfiles[i].processed_size = logfiles[i].size;
		logfiles[i].seq = seq++;
	}

	if (0 < *logfiles_num_old && 0 < logfiles_num)
	{
		/* set up a mapping array from old files to new files */
		old2new =   _malloc(old2new, (size_t)logfiles_num * (size_t)(*logfiles_num_old) * sizeof(char));

		if (SUCCEED != setup_old2new(old2new, *logfiles_old, *logfiles_num_old, logfiles, logfiles_num,
				*use_ino))
		{
			destroy_logfile_list(&logfiles, &logfiles_alloc, &logfiles_num);
			  _free(old2new);
			(*error_count)++;
			ret = SUCCEED;
			goto out;
		}

		if (1 < *logfiles_num_old || 1 < logfiles_num)
			resolve_old2new(old2new, *logfiles_num_old, logfiles_num);

		/* Transfer data about fully and partially processed files from the old file list to the new list. */
		for (i = 0; i < *logfiles_num_old; i++)
		{
			if (0 < (*logfiles_old)[i].processed_size && 0 == (*logfiles_old)[i].incomplete &&
					-1 != (j = find_old2new(old2new, logfiles_num, i)))
			{
				if ((*logfiles_old)[i].size == (*logfiles_old)[i].processed_size
						&& (*logfiles_old)[i].size == logfiles[j].size)
				{
					/* the file was fully processed during the previous check and must be ignored */
					/* during this check */
					logfiles[j].processed_size = logfiles[j].size;
					logfiles[j].seq = seq++;
				}
				else
				{
					/* the file was not fully processed during the previous check or has grown */
					logfiles[j].processed_size = (*logfiles_old)[i].processed_size;
				}
			}
			else if (1 == (*logfiles_old)[i].incomplete &&
					-1 != (j = find_old2new(old2new, logfiles_num, i)))
			{
				if ((*logfiles_old)[i].size < logfiles[j].size)
				{
					/* The file was not fully processed because of incomplete last record */
					/* but it has grown. Try to process it further. */
					logfiles[j].incomplete = 0;
				}
				else
					logfiles[j].incomplete = 1;

				logfiles[j].processed_size = (*logfiles_old)[i].processed_size;
			}

			/* find the last file processed (fully or partially) in the previous check */
			if (max_old_seq < (*logfiles_old)[i].seq)
			{
				max_old_seq = (*logfiles_old)[i].seq;
				old_last = i;
			}
		}

		/* find the first file to continue from in the new file list */
		if (0 < max_old_seq && -1 == (start_idx = find_old2new(old2new, logfiles_num, old_last)))
		{
			/* Cannot find the successor of the last processed file from the previous check. */
			/* 'lastlogsize' does not apply in this case. */
			*lastlogsize = 0;
			start_idx = 0;
		}
	}

	  _free(old2new);

	if (SUCCEED ==  _check_log_level(LOG_LEVEL_DEBUG))
	{
		 _log(LOG_LEVEL_DEBUG, "process_logrt() old file list:");
		if (NULL != *logfiles_old)
			print_logfile_list(*logfiles_old, *logfiles_num_old);
		else
			 _log(LOG_LEVEL_DEBUG, "   file list empty");

		 _log(LOG_LEVEL_DEBUG, "process_logrt() new file list: (mtime:%d lastlogsize:"   _FS_UI64
				" start_idx:%d)", *mtime, *lastlogsize, start_idx);
		if (NULL != logfiles)
			print_logfile_list(logfiles, logfiles_num);
		else
			 _log(LOG_LEVEL_DEBUG, "   file list empty");
	}

	/* forget the old logfile list */
	if (NULL != *logfiles_old)
	{
		for (i = 0; i < *logfiles_num_old; i++)
			  _free((*logfiles_old)[i].filename);

		*logfiles_num_old = 0;
		  _free(*logfiles_old);
	}

	/* enter the loop with index of the first file to be processed, later continue the loop from the start */
	i = start_idx;

	/* from now assume success - it could be that there is nothing to do */
	ret = SUCCEED;

	while (i < logfiles_num)
	{
		if (0 == logfiles[i].incomplete && (logfiles[i].size != logfiles[i].processed_size ||
				0 == logfiles[i].seq))
		{
			if (start_idx != i)
				*lastlogsize = logfiles[i].processed_size;

			ret = process_log(logfiles[i].filename, lastlogsize, (1 == is_logrt) ? mtime : NULL,
					skip_old_data, big_rec, &logfiles[i].incomplete, encoding, regexps, pattern,
					output_template, p_count, s_count, process_value, server, port, hostname, key);

			/* process_log() advances 'lastlogsize' only on success therefore */
			/* we do not check for errors here */
			logfiles[i].processed_size = *lastlogsize;

			/* Mark file as processed (at least partially). In case if process_log() failed we will stop */
			/* the current checking. In the next check the file will be marked in the list of old files */
			/* and we will know where we left off. */
			logfiles[i].seq = seq++;

			if (SUCCEED != ret)
			{
				(*error_count)++;
				ret = SUCCEED;
				break;
			}

			if (0 >= *p_count || 0 >= *s_count)
			{
				ret = SUCCEED;
				break;
			}
		}

		if (0 != from_first_file)
		{
			/* We have processed the file where we left off in the previous check. */
			from_first_file = 0;

			/* Now proceed from the beginning of the new file list to process the remaining files. */
			i = 0;
			continue;
		}

		i++;
	}

	/* remember the current logfile list */
	*logfiles_num_old = logfiles_num;

	if (0 < logfiles_num)
		*logfiles_old = logfiles;
out:
	 _log(LOG_LEVEL_DEBUG, "End of %s():%s error_count:%d", __function_name,   _result_string(ret),
			*error_count);

	return ret;
}
