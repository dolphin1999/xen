#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/errno.h>
#include <linux/slab.h>
#include <asm/hypervisor-ifs/block.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>

static struct proc_dir_entry *phd;

extern int xenolinux_control_msg(int operration, char *buffer, int size);

static ssize_t proc_read_phd(struct file * file, char * buff, size_t size, loff_t * off)
{
  physdisk_probebuf_t *buf;
  int res;

  if (size != sizeof(physdisk_probebuf_t))
    return -EINVAL;

  buf = kmalloc(sizeof(physdisk_probebuf_t), GFP_KERNEL);
  if (!buf)
    return -ENOMEM;

  if (copy_from_user(buf, buff, size)) {
    kfree(buf);
    return -EFAULT;
  }

  printk("max aces 1 %x\n", buf->n_aces);

  res = xenolinux_control_msg(XEN_BLOCK_PHYSDEV_PROBE, (void *)buf,
			      sizeof(physdisk_probebuf_t));

  printk("max aces %x\n", buf->n_aces);

  if (res)
    res = -EINVAL;
  else {
    res = sizeof(physdisk_probebuf_t);
    if (copy_to_user(buff, buf, sizeof(physdisk_probebuf_t))) {
      res = -EFAULT;
    }
  }
  kfree(buf);
  return res;
}

static int proc_write_phd(struct file *file, const char *buffer,
			  size_t count, loff_t *ignore)
{
  char *local;
  int res;

  if (count != sizeof(xp_disk_t))
    return -EINVAL;

  local = kmalloc(count + 1, GFP_KERNEL);
  if (!local)
    return -ENOMEM;
  if (copy_from_user(local, buffer, count)) {
    res = -EFAULT;
    goto out;
  }
  local[count] = 0;

  res = xenolinux_control_msg(XEN_BLOCK_PHYSDEV_GRANT, local, count);
  if (res == 0)
    res = count;
  else
    res = -EINVAL;
 out:
  kfree(local);
  return res;
}

static struct file_operations proc_phd_fops = {
  read : proc_read_phd,
  write : proc_write_phd
};

int __init xlphysdisk_proc_init(void)
{
  phd = create_proc_entry("xeno/dom0/phd", 0644, NULL);
  if (!phd) {
    panic("Can\'t create phd proc entry!\n");
  }
  phd->data = NULL;
  phd->proc_fops = &proc_phd_fops;
  phd->owner = THIS_MODULE;

  return 0;
}
