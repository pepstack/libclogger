#!/bin/bash
#
# @file revise-source.sh
#   revise source file version
#
# @author mapaware@hotmail.com
# @copyright mapaware.top
# @version 1.0.0
#
# @since 2024-10-08 19:24:50
# @date 2024-11-04 13:07:30
#
# usage:
#
#   $ revise-source.sh "/path/to/mycode.c"
#######################################################################
_name_=$(basename "$0")
_cdir_=$(cd "$(dirname "$0")" && pwd)
_file_=""${_cdir_}"/"${_name_}""
_ver_="0.1.1"
#######################################################################
# Treat unset variables as an error
set -o nounset

# Treat any error as exit
set -o errexit
#######################################################################
# !! 此段内容不能更改 !!
PathFile="$1"
FileName=$(basename "$PathFile")
PathDir=$(cd "$(dirname "$PathFile")" && pwd)

#### 自动创建工程模板
function checkProjectFiles() {
    if [ ! -d "${_cdir_}/source" ]; then
        mkdir -p "${_cdir_}/source/common"
    fi

    if [ ! -d "${_cdir_}/.vscode" ]; then
        mkdir "${_cdir_}/.vscode"
        > "${_cdir_}/.vscode/c_cpp_properties.json"
        > "${_cdir_}/.vscode/launch.json"
        > "${_cdir_}/.vscode/settings.json"
        > "${_cdir_}/.vscode/tasks.json"
    fi

    if [ ! -f "${_cdir_}/Makefile" ]; then
        > "${_cdir_}/Makefile"
    fi

    if [ ! -f "${_cdir_}/COPYRIGHT" ]; then
        echo "© 2024-2030 mapaware.top All Rights Reserved." >> "${_cdir_}/COPYRIGHT"
        echo "COPYRIGHT file created"
    fi

    if [ ! -f "${_cdir_}/AUTHOR" ]; then
        echo "mapaware@hotmail.com" >> "${_cdir_}/AUTHOR"
        echo "AUTHOR file created"
    fi

    if [ ! -f "${_cdir_}/VERSION" ]; then
        echo "0.0.1" >> "${_cdir_}/VERSION"
        echo "VERSION file created"
    fi
}

checkProjectFiles

COPYRIGHT="$(sed '1q;d' "${_cdir_}/COPYRIGHT")"
AUTHOR="$(sed '1q;d' "${_cdir_}/AUTHOR")"
VERSION="$(sed '1q;d' "${_cdir_}/VERSION")"

# 确保不会处理目录!!
if [ -d "$PathFile" ]; then
    echo "Error: CAN NOT be a path: $PathFile"
    exit
fi

if [ ! -f "$PathFile" ]; then
    echo "Error: File not found: $PathFile"
    exit
fi

if [ "$FileName" = "revise-source.sh" ]; then
    # Ignored this self file
    exit
fi

#######################################################################
# !! 此段内容不能更改 !!
printf "%-40s@%s\n" "* ${FileName}" "$PathDir"

# 取得文件最后访问时间
accessTime=$(stat -c %x "${PathFile}")
accessTimeFmt="$(date -d "$accessTime" +"%Y-%m-%d %H:%M:%S")"

# 取得文件最后修改时间
modifyTime=$(stat -c %y "${PathFile}")
modifyTimeFmt="$(date -d "$modifyTime" +"%Y-%m-%d %H:%M:%S")"

# 取得文件创建时间. Linux 不提供
createTime=$(stat -c %w "${PathFile}")
if [ "$createTime" = "-" ]; then
    # 以文件最后修改时间为创建时间
    createTimeFmt="$modifyTimeFmt"
else
    createTimeFmt="$(date -d "$createTime" +"%Y-%m-%d %H:%M:%S")"
fi

# 取最后更改日期: 2024-10-08 23:54:30
fileLastDate=$(sed -n '/^# @date\S*/p' "${PathFile}" | awk '{print $3" "$4}')
if [ -z "$fileLastDate" ]; then
    fileLastDate=$(sed -n '/^\*\* @date\S*/p' "${PathFile}" | awk '{print $3" "$4}')
fi

# 取版本
fileLastVersion=$(sed -n '/^# @version\S*/p' "${PathFile}" | awk '{print $3}')
if [ -z "$fileLastVersion" ]; then
    fileLastVersion=$(sed -n '/^\*\* @version\S*/p' "${PathFile}" | awk '{print $3}')
fi
######## 下面开始修复文件版本 ########
# 行尾 CRLF 更改为 LF
sed -i 's/\r//g' "${PathFile}"

# 剔除行尾空白字符
sed -i 's/[[:space:]]*$//' "${PathFile}"

#### shell/python/Makefile
# @file
sed -i 's/^# @file\s*$/# @file      '"${FileName}"'/' "${PathFile}"

# @author mapaware@hotmail.com
sed -i 's/^# @author\s*$/# @author   '"${AUTHOR}"'/' "${PathFile}"

# @copyright © 2024-2030 mapaware.top All Rights Reserved.
sed -i 's/^# @copyright\s*$/# @copyright '"${COPYRIGHT}"'/' "${PathFile}"

# @since 文件创建时间
sed -i 's/^# @since\s*$/# @since     '"${createTimeFmt}"'/' "${PathFile}"

# @date 2024-10-08 23:54:30
sed -i 's/^# @date\s*$/# @date      '"${modifyTimeFmt}"'/' "${PathFile}"

# @version 版本
sed -i 's/^# @version\s*$/# @version   '"${VERSION}"'/' "${PathFile}"

#### c/cpp/java/css
# @file
sed -i 's/^\*\* @file\s*$/\*\* @file       '"${FileName}"'/' "${PathFile}"

# @author mapaware@hotmail.com
sed -i 's/^\*\* @author\s*$/\*\* @author    '"${AUTHOR}"'/' "${PathFile}"

# @copyright © 2024-2030 mapaware.top All Rights Reserved.
sed -i 's/^\*\* @copyright\s*$/\*\* @copyright '"${COPYRIGHT}"'/' "${PathFile}"

# @since 文件创建时间
sed -i 's/^\*\* @since\s*$/\*\* @since     '"${createTimeFmt}"'/' "${PathFile}"

# @date 2024-10-08 23:54:30
sed -i 's/^\*\* @date\s*$/\*\* @date      '"${modifyTimeFmt}"'/' "${PathFile}"

# @version 版本
sed -i 's/^\*\* @version\s*$/\*\* @version    '"${VERSION}"'/' "${PathFile}"

#### 自动升级文件版本号
if [ ! -z "$fileLastVersion" ]; then
    if [[ "$modifyTimeFmt" > "$fileLastDate" ]]; then
        # 自最后一次版本更新, 文件发生修改
        fileCurrVersion=$(sed -n '/^# @version\S*/p' "${PathFile}" | awk '{print $3}')
        if [ -z "$fileCurrVersion" ]; then
            fileCurrVersion=$(sed -n '/^\*\* @version\S*/p' "${PathFile}" | awk '{print $3}')
        fi
        if [ ! -z "$fileCurrVersion" ]; then
            if [[ "$fileCurrVersion" > "$fileLastVersion" ]]; then
                echo "    + version revised: $fileLastVersion => $fileCurrVersion"
            else
                reviseNum=$(echo "$fileCurrVersion" | awk -F[.] '{print $3}')
                minorNum=$(echo "$fileCurrVersion" | awk -F[.] '{print $2}')
                majorNum=$(echo "$fileCurrVersion" | awk -F[.] '{print $1}')

                # 版本号自动升级: 1.2.35 => 1.2.36, 1.8.99 => 1.9.0, 1.9.99 => 2.0.0
                if [ "$reviseNum" -lt 99 ]; then
                    reviseNum=$((reviseNum+1))
                elif [ "$minorNum" -lt 9 ]; then
                    minorNum=$((minorNum+1))
                    reviseNum=0
                else
                    majorNum=$((majorNum+1))
                    minorNum=0
                    reviseNum=0        
                fi

                fileNewVersion="$majorNum"."$minorNum"."$reviseNum"
                echo "    + version revised: $fileCurrVersion => $fileNewVersion"

                sed -i 's/^# @version\s*$/# @version   '"${fileNewVersion}"'/' "${PathFile}"
                sed -i 's/^\*\* @version\s*$/\*\* @version   '"${fileNewVersion}"'/' "${PathFile}"
            fi
        fi
    fi
fi

#### 最后需要恢复文件的时间
touch -a -c -d "${accessTime}" "${PathFile}"
touch -m -c -d "${modifyTime}" "${PathFile}"

exit 0