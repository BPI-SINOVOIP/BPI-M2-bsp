#ifndef __DMA_CLASS_H__
#define __DMA_CLASS_H__
struct mutex sunxi_dma_test_mutex;
struct sunxi_dma_test_class{
	char					exec;
	char					update;
	unsigned char			channel_no;
	char					channel_number;
	//struct	dma_config_t	information;
	struct device		   	*dev;
};

struct dma_test_inrotmation_array{
	char	number;
	struct	dma_config_t	information;
};

struct test_result{
	char	dma_qd_fd_hd_flag;
	char	end_flag;
	char	*name;
	char	result;
	void	*vsrcp;
	void	*vdstp;
	int		loop_times;
};
#endif
