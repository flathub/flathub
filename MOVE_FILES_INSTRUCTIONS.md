# Flathub PR 修复说明

## 当前问题
PR 被阻止，原因是：

1. **Flatpak manifest 不在顶层** - ❌
   - 当前位置：`com.github.lizl6.canbox/com.github.lizl6.canbox.json`
   - 需要位置：`com.github.lizl6.canbox.json`（仓库顶层）

2. **Metainfo 文件不在顶层** - ❌
   - 当前位置：`com.github.lizl6.canbox/com.github.lizl6.canbox.metainfo.xml`
   - 需要位置：`com.github.lizl6.canbox.metainfo.xml`（仓库顶层）

3. **Appstream 目录不在顶层** - ❌
   - 当前位置：`com.github.lizl6.canbox/appstream/`
   - 需要位置：`appstream/`（仓库顶层）

## 修复步骤

请在终端执行以下命令：

```bash
cd /depot/cargo/flathub

# 1. 复制 appstream 目录到顶层
cp -r com.github.lizl6.canbox/appstream .

# 2. 验证文件结构
ls -la

# 3. 删除旧的子目录
rm -rf com.github.lizl6.canbox

# 4. 验证最终结构
tree -L 2
# 或者
ls -la
```

## 修复后的正确结构

```
flathub/
├── com.github.lizl6.canbox.json          ← Manifest 文件（顶层）
├── com.github.lizl6.canbox.metainfo.xml  ← Metainfo 文件（顶层）
└── appstream/                            ← Appstream 资源（顶层）
    └── screenshots/
        ├── screenshot-1.png
        ├── screenshot-2.png
        ├── screenshot-3.png
        ├── screenshot-4.png
        └── screenshot-5.png
```

## 其他需要确认的事项

### 1. 更新 PR 描述

确保 PR 描述中所有 checklist 都已完成（打勾 `[X]`）：

```markdown
- [X] Please describe the application briefly. [已填写]
- [ ] Please attach a video showcasing the application on Linux using the Flatpak. [需要上传视频]
- [X] The Flatpak ID follows all the rules listed in the Application ID requirements.
- [X] I have read and followed all the Submission requirements and the Submission guide and I agree to them.
- [X] I am an author/developer to the project.
```

### 2. 上传演示视频

录制并上传一个演示视频，展示 Canbox 在 Linux 上的运行情况，然后将视频链接添加到 PR 描述中。

## 提交更改

修复文件结构后：

```bash
cd /depot/cargo/flathub

# 添加所有更改
git add .
git commit -m "fix: move manifest and metadata files to toplevel"

# 推送到你的 fork
git push origin 你的分支名
```

## 重新检查 PR

提交后，Flathub 的自动化检查会自动运行，检查：
- ✅ Manifest 文件在顶层
- ✅ Metainfo 文件在顶层
- ✅ Checklist 完整
- ✅ 符合提交要求

等待检查通过即可。
