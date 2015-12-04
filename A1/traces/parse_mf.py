#!/usr/bin/python

import sys

def main(*arg):
    fin = open('7-longrandom', 'r')
    fout = open('7-arranged', 'w')
    lines = fin.readlines()
    m_lines = []
    f_lines = []
    for i in range(len(lines)):
        if 'm' in lines[i]:
            m_lines.append(lines[i])
        if 'f' in lines[i]:
            f_lines.append(lines[i])
    for ele in m_lines:
        fout.write(ele)
    for ele in f_lines:
        fout.write(ele)
    fin.close()
    fout.close()


if __name__ == "__main__":
    main()
    
