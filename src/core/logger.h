
#pragma once

#include "config.h"
#include "error.h"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "jrtplib3/rtperrors.h"

namespace rtpserver {

/**
 * @brief The Logger class
 * 日志模块，输出日志
 * Debug模式默认输出到控制台
 * Release模式默认输出到文件
 */
class Logger
{
public:
	Logger() = delete;
	
	/**
	 * @brief InitLogger
	 * 初始化程序需要用到的日志器
	 */
	static inline void Init_logger(){
		if(init)
			return;
		spdlog::init_thread_pool(4096,2);
	#ifdef DEBUG 
		auto ptr = spdlog::create_async_nb<spdlog::sinks::stdout_sink_mt>(LOGGERNAME);
		spdlog::create_async_nb<spdlog::sinks::stderr_sink_mt>(LOGGER_ERROR_NAME);
		
	#else
		auto ptr = spdlog::create_async_nb<spdlog::sinks::rotating_file_sink_mt>(
					LOGGERNAME,LOGGERFILENAME,4096,3);
	#endif
		init = true;
		{
			ptr->set_pattern(" [%C-%m-%d %H:%M:%S:%e] %v ***");
			ptr->info("Initialization log module");
		}
		spdlog::set_pattern(" [%C-%m-%d %H:%M:%S:%e] [%7l] [%5t] %v ***");
	}
	
	
	/**
	 * @brief Info
	 * 输出一般信息
	 * @param msg
	 * 信息
	 * @param api
	 * 信息具体出现的位置
	 */
	template<typename ... _T>
	static inline void Info(const char * msg,const char * api,const _T & ...t){
		Init_logger();
		auto ptr = spdlog::get(LOGGERNAME);
		ptr->info(Insert_api_format(msg).c_str(),api,t...);
		ptr->flush();
	}
	
	/**
	 * @brief Warning
	 * 输出警告
	 * @param msg
	 * @param api
	 * 信息具体出现的位置
	 */
	template<typename ... _T>
	static inline void Warning(const char * msg,const char * api, const _T & ... t){
		Init_logger();
		auto ptr = spdlog::get(LOGGERNAME);
		ptr->warn(Insert_api_format(msg).c_str(),api,t...);
		ptr->flush();
	}
	
	/**
	 * @brief Error
	 * 输出错误
	 * @param msg
	 * @param api
	 * 信息具体出现的位置
	 */
	template<typename ... _T>
	static inline void Error(const char * msg,const char * api, const _T & ... t){
		Init_logger();
	#ifdef DEBUG 
		auto ptr = spdlog::get(LOGGER_ERROR_NAME);
	#else
		auto ptr = spdlog::get(LOGGERNAME);
	#endif
		ptr->error(Insert_api_format(msg).c_str(),api,t...);
		ptr->flush();
	}
	
	/**
	 * @brief Error_rtp
	 * 输出错误(对应jrtplib的错误码)
	 * @param num
	 * 错误代码
	 * @param api
	 * 调用该接口的API
	 */
	static inline void Error_rtp(int num,const char * api){
		Error(jrtplib::RTPGetErrorString(num).c_str(),api);
	}
	
	/**
	 * @brief Error_app
	 * 输出错误(程序的错误代码)
	 * @param num
	 * 错误代码
	 * @param api
	 * 调用该接口的api
	 */
	template<typename ... _T>
	static inline void Error_app(MessageNum num,const char * api,const _T &... t){
		Error(MessageString[static_cast<int>(num)],api,t...);
	}
	
	/**
	 * @brief Info_app
	 * 和上面的接口没区别
	 * @param num
	 * @param api
	 */
	template<typename ... _T>
	static inline void Info_app(MessageNum num,const char *api,const _T& ... t){
		Info(MessageString[static_cast<int>(num)],api,t...);
	}

	
	/**
	 * @brief Warning_app
	 * 和上面的接口没区别
	 * @param num
	 * @param api
	 */
	template<typename ... _T>
	static inline void Warning_app(MessageNum num,const char *api,const _T& ... t){
		Warning(MessageString[static_cast<int>(num)],api,t...);
	}
	
	/**
	 * @brief ClearAll
	 * 关闭所有日志
	 */
	static inline void Clear_all(){
		if(!init)
			return;
		auto ptr = spdlog::get(LOGGERNAME);
		ptr->set_pattern(" [%C-%m-%d %H:%M:%S:%e] %v ***");
		ptr->info("Close log module");
		spdlog::drop_all();
		spdlog::shutdown();
		init = false;
	}
private:
	static inline const std::string Insert_api_format(const char * msg){
		std::string str("[{}] ");
		return str.append(msg);
	}
private:
	static bool init;
};

} // namespace rtpserver

