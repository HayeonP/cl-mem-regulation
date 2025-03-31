import os

def parse_ps_with_grep(grep_str):
    ps_info_list = []
    _output = str(os.popen('ps ax | grep ' + grep_str).read()).split('\n')
    for line in _output:
        ps_info = line.split(' ')
        ps_info = [v for v in ps_info if v != '']
        ps_info_list.append(ps_info)
    return ps_info_list

def kill_processes_with_grep(grep_str):
    ps_info_list = parse_ps_with_grep(grep_str)

    for ps_info in ps_info_list:
        if len(ps_info) == 0: continue
        if 'grep' in ps_info: continue
        
        pid = ps_info[0]
        os.system('kill -15 ' + pid)

if __name__ == '__main__':
    kill_processes_with_grep('gpu_profiler')