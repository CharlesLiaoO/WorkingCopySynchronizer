# Working Copy Synchronizer (WCS)
Synchronizer of version control system's working copy

When you write the cross platform codes with vcs (version control system), in order to synchronize code between platforms efficiently, you may do these:
* First: Use vcs client in every platform, synchronize codes through server of vcs.
* Second: Use only one working copy and one vcs client in a platform, then use Network Shared Folder to synchronize codes to other platforms.

But, these manners have shortcomings:
* First manner: You have to commit frequently between platforms, which may cause the vcs commits messy.
* Second manner: The only one working copy directory may cause the same name conflicts of the build output.
  Even badly, the file system of the directory may don't support file type of another platform, 
  such as ntfs (Windows file system) do not support softlink (file type of unix)

So I write this Working Copy Synchronizer (WCS) to improve this situation.

WCS runs in one platform, depends on a 'Master working copy directory' (MasterCopyDir) of vcs, then queries all files in the MasterCopyDir tracked by vcs.
Then WCS compares the modified time fo the file; if the modified time differed, 
WCS copys file between MasterCopyDir and the 'Branch working copy directory' (BranchCopyDir) in other platform through Network Shared Folder.

Now, WCS supports:
* VCS: Git, SVN
* One MasterCopyDir and one BranchCopyDir
* Synchronizing every 2s, or once by one start trigger
