# python code used for testing pseudo-code

def ext_ls(path):
	# path input should be the format /dir1/dir2/file or /dir1/dir2
	# output a list of string giving different layers
	if path[0] != '/':
		print 'should start from /'
		return
	# split path into layers
	layers = path.split('/')
	# remove None in the layers list
	layers = filter(None, layers)
	return layers




def main(*arg):
	path = arg[0]
	print ext_ls(path)


if __name__ == '__main__':
	main('/dir1/dir2/dir3/abc.txt')
